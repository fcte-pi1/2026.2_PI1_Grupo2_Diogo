import asyncio
import logging
from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.encoders import jsonable_encoder
from pydantic import TypeAdapter, ValidationError
from sqlalchemy import text
from sqlalchemy.exc import SQLAlchemyError
from sqlalchemy.orm import Session

from app.core.config import settings
from app.core.database import SessionLocal, get_db
from app.schemas.telemetry import (
    EspTelemetryInput,
    MetricsData,
    PositionData,
    TelemetryData,
)
from app.services.telemetry_service import telemetry_service
from app.services.esp_websocket import esp_websocket_manager


router = APIRouter(
    prefix="/telemetry",
    tags=["Telemetry"],
)
websocket_router = APIRouter(tags=["Telemetry"])
telemetry_input_adapter = TypeAdapter(TelemetryData | EspTelemetryInput)
logger = logging.getLogger(__name__)


@router.get("/status")
def get_esp_connection_status():
    """Estado de conexão inferido pelo recebimento recente de pacotes da ESP."""
    return telemetry_service.get_connection_status()

@router.get("/current")
def get_current_telemetry(db: Session = Depends(get_db)):
    """Consulta a última leitura válida; recupera do banco após reinício."""
    try:
        return _get_current_telemetry(db)
    except SQLAlchemyError:
        db.rollback()
        logger.warning("Consulta da telemetria indisponível.", exc_info=True)
        return {
            **telemetry_service.get_snapshot(),
            "detail": "Telemetria temporariamente indisponível.",
        }


def _get_current_telemetry(db: Session):
    current = telemetry_service.get_snapshot()
    if current["has_data"]:
        return current

    sample = db.execute(
        text(
            "SELECT t.id AS telemetry_sample_id, t.execucao_id, "
            "t.tempo_relativo_ms, t.nivel_bateria_pct, t.consumo_eletrico_ma, "
            "t.velocidade_instantanea_cm_s, t.rpm_motor_esq, t.rpm_motor_dir, "
            "m.giro_x_deg_s, m.giro_y_deg_s, m.giro_z_deg_s, m.pos_x, m.pos_y, "
            "e.iniciada_em + (t.tempo_relativo_ms * INTERVAL '1 millisecond') "
            "AS generated_at "
            "FROM amostra_telemetria t "
            "JOIN execucao e ON e.id = t.execucao_id "
            "LEFT JOIN LATERAL ("
            "  SELECT giro_x_deg_s, giro_y_deg_s, giro_z_deg_s, pos_x, pos_y "
            "  FROM amostra_movimento "
            "  WHERE execucao_id = t.execucao_id "
            "    AND tempo_relativo_ms = t.tempo_relativo_ms "
            "  ORDER BY id DESC LIMIT 1"
            ") m ON TRUE "
            "ORDER BY (e.status = 'em_execucao') DESC, t.id DESC LIMIT 1"
        )
    ).mappings().first()

    if sample is not None:
        movement = {
            key: sample[key]
            for key in ("giro_x_deg_s", "giro_y_deg_s", "giro_z_deg_s")
        }
        telemetry_service.restore(
            TelemetryData(
                position=PositionData(
                    row=sample["pos_y"] or 0, col=sample["pos_x"] or 0
                ),
                metrics=MetricsData(
                    speed_cm_s=sample["velocidade_instantanea_cm_s"],
                    rpm_left=sample["rpm_motor_esq"],
                    rpm_right=sample["rpm_motor_dir"],
                    battery_pct=sample["nivel_bateria_pct"],
                ),
                movement=(
                    movement if all(value is not None for value in movement.values())
                    else None
                ),
                generated_at=sample["generated_at"],
            ),
            execucao_id=sample["execucao_id"],
            telemetry_sample_id=sample["telemetry_sample_id"],
            tempo_relativo_ms=sample["tempo_relativo_ms"],
        )
    return telemetry_service.get_snapshot()


@router.post("")
def receive_telemetry(
    telemetry: EspTelemetryInput | TelemetryData,
    db: Session = Depends(get_db),
):
    """Recebe uma leitura; persiste quando há uma corrida ativa."""
    try:
        return _save_telemetry(telemetry, db)
    except SQLAlchemyError as error:
        db.rollback()
        logger.warning("Gravação da telemetria indisponível.", exc_info=True)
        raise HTTPException(
            status_code=503,
            detail="Leitura recebida, mas não foi possível gravá-la.",
        ) from error


def _save_telemetry(telemetry: EspTelemetryInput | TelemetryData, db: Session):
    needs_stored_position = False
    reported_connected = (
        telemetry.connected if isinstance(telemetry, EspTelemetryInput) else True
    )
    telemetry_service.mark_esp_packet_received(reported_connected)
    if isinstance(telemetry, EspTelemetryInput):
        cached_position = telemetry_service.get_snapshot()["position"]
        position = telemetry.position
        if position is None and telemetry.row is not None:
            position = PositionData(row=telemetry.row, col=telemetry.col)
        if position is None:
            needs_stored_position = cached_position is None
            position = PositionData(**(cached_position or {"row": 0, "col": 0}))
        telemetry = TelemetryData(
            position=position,
            metrics=MetricsData(
                battery_pct=telemetry.battery_pct,
                speed_cm_s=telemetry.speed_cm_s,
                rpm=telemetry.rpm,
            ),
            generated_at=datetime.now(timezone.utc),
        )
    if not telemetry_service.update(telemetry):
        return {"status": "ignored", "saved": False, "detail": "Amostra fora de ordem."}

    execution = db.execute(
        text(
            "SELECT id, FLOOR(EXTRACT(EPOCH FROM "
            "(LOCALTIMESTAMP - iniciada_em)) * 1000)::integer AS elapsed_ms "
            "FROM execucao "
            "WHERE status = 'em_execucao' ORDER BY iniciada_em DESC LIMIT 1"
        )
    ).mappings().first()

    if execution is None:
        return {"status": "received", "saved": False, "execucao_id": None}

    elapsed_ms = max(0, execution["elapsed_ms"])
    telemetry_service.associate_execution(
        telemetry, execucao_id=execution["id"], tempo_relativo_ms=elapsed_ms
    )
    battery = telemetry.metrics.battery_pct
    speed = telemetry.metrics.speed_cm_s
    rpm_left = int(
        telemetry.metrics.rpm_left
        if telemetry.metrics.rpm_left is not None else telemetry.metrics.rpm
    )
    rpm_right = int(
        telemetry.metrics.rpm_right
        if telemetry.metrics.rpm_right is not None else telemetry.metrics.rpm
    )
    pos_x = telemetry.position.col
    pos_y = telemetry.position.row
    movement = telemetry.movement
    gyro = (0, 0, 0) if movement is None else (
        movement.giro_x_deg_s, movement.giro_y_deg_s, movement.giro_z_deg_s
    )

    last_telemetry = db.execute(
        text(
            "SELECT tempo_relativo_ms, nivel_bateria_pct, "
            "velocidade_instantanea_cm_s, rpm_motor_esq, rpm_motor_dir "
            "FROM amostra_telemetria WHERE execucao_id = :execucao_id "
            "ORDER BY id DESC LIMIT 1"
        ),
        {"execucao_id": execution["id"]},
    ).mappings().first()
    last_movement = db.execute(
        text(
            "SELECT pos_x, pos_y, giro_x_deg_s, giro_y_deg_s, giro_z_deg_s FROM amostra_movimento "
            "WHERE execucao_id = :execucao_id ORDER BY id DESC LIMIT 1"
        ),
        {"execucao_id": execution["id"]},
    ).mappings().first()

    if needs_stored_position and last_movement is not None:
        pos_x, pos_y = last_movement["pos_x"], last_movement["pos_y"]
        telemetry.position = PositionData(row=pos_y, col=pos_x)
        telemetry_service.complete_position(telemetry)

    if last_telemetry is not None and last_movement is not None:
        unchanged = (
            round(last_telemetry["nivel_bateria_pct"], 1) == round(battery, 1)
            and round(last_telemetry["velocidade_instantanea_cm_s"], 2) == round(speed, 2)
            and last_telemetry["rpm_motor_esq"] == rpm_left
            and last_telemetry["rpm_motor_dir"] == rpm_right
            and last_movement["pos_x"] == pos_x
            and last_movement["pos_y"] == pos_y
            and tuple(
                last_movement[key]
                for key in ("giro_x_deg_s", "giro_y_deg_s", "giro_z_deg_s")
            ) == gyro
        )
        time_since_last_sample_ms = elapsed_ms - last_telemetry["tempo_relativo_ms"]
        if (
            unchanged
            and time_since_last_sample_ms < settings.TELEMETRY_HEARTBEAT_INTERVAL_MS
        ):
            return {
                "status": "unchanged",
                "saved": False,
                "execucao_id": execution["id"],
                "next_sample_in_ms": max(
                    0,
                    settings.TELEMETRY_HEARTBEAT_INTERVAL_MS
                    - time_since_last_sample_ms,
                ),
            }

    inserted_id = db.execute(
        text(
            "INSERT INTO amostra_telemetria ("
            "execucao_id, tempo_relativo_ms, nivel_bateria_pct, "
            "consumo_eletrico_ma, velocidade_instantanea_cm_s, rpm_motor_esq, rpm_motor_dir"
            ") VALUES ("
            ":execucao_id, :tempo_relativo_ms, :bateria, "
            "0, :velocidade, :rpm_left, :rpm_right"
            ") RETURNING id"
        ),
        {
            "execucao_id": execution["id"],
            "tempo_relativo_ms": elapsed_ms,
            "bateria": battery,
            "velocidade": speed,
            "rpm_left": rpm_left,
            "rpm_right": rpm_right,
        },
    ).scalar_one()
    movement_id = db.execute(
        text(
            "INSERT INTO amostra_movimento ("
            "execucao_id, tempo_relativo_ms, giro_x_deg_s, giro_y_deg_s, "
            "giro_z_deg_s, pos_x, pos_y"
            ") VALUES (:execucao_id, :tempo_relativo_ms, :gyro_x, :gyro_y, :gyro_z, :pos_x, :pos_y) "
            "RETURNING id"
        ),
        {
            "execucao_id": execution["id"],
            "tempo_relativo_ms": elapsed_ms,
            "pos_x": pos_x,
            "pos_y": pos_y,
            "gyro_x": gyro[0],
            "gyro_y": gyro[1],
            "gyro_z": gyro[2],
        },
    ).scalar_one()
    db.execute(
        text(
            "UPDATE execucao SET quantidade_celulas_visitadas = ("
            "  SELECT COUNT(*) FROM ("
            "    SELECT DISTINCT pos_x, pos_y FROM amostra_movimento "
            "    WHERE execucao_id = :execucao_id"
            "  ) AS celulas_visitadas"
            ") WHERE id = :execucao_id"
        ),
        {"execucao_id": execution["id"]},
    )
    db.commit()
    telemetry_service.associate_execution(telemetry, telemetry_sample_id=inserted_id)

    return {
        "status": "saved",
        "saved": True,
        "execucao_id": execution["id"],
        "telemetry_sample_id": inserted_id,
        "movement_sample_id": movement_id,
    }


@websocket_router.websocket("/ws/telemetry")
async def receive_telemetry_websocket(websocket: WebSocket):
    """Recebe amostras JSON da ESP por uma conexão WebSocket persistente."""
    await websocket.accept()
    esp_websocket_manager.connect(websocket)
    try:
        while True:
            try:
                payload = await websocket.receive_json()
                telemetry = telemetry_input_adapter.validate_python(payload)
            except (ValidationError, ValueError, TypeError) as error:
                await esp_websocket_manager.send_json({"status": "error", "detail": str(error)})
                continue

            try:
                result = await asyncio.to_thread(_save_websocket_sample, telemetry)
                await esp_websocket_manager.send_json(result)
            except HTTPException as error:
                await esp_websocket_manager.send_json(
                    {"status": "error", "detail": error.detail, "code": error.status_code}
                )
            except SQLAlchemyError:
                logger.warning("Gravação da telemetria indisponível.", exc_info=True)
                await esp_websocket_manager.send_json(
                    {
                        "status": "error", "saved": False, "code": 503,
                        "detail": "Leitura recebida, mas não foi possível gravá-la.",
                    }
                )
    except WebSocketDisconnect:
        pass
    finally:
        esp_websocket_manager.disconnect(websocket)


def _save_websocket_sample(telemetry: TelemetryData | EspTelemetryInput):
    with SessionLocal() as db:
        return _save_telemetry(telemetry, db)


def _read_websocket_snapshot():
    with SessionLocal() as db:
        return get_current_telemetry(db)


@websocket_router.websocket("/ws/telemetry/current")
async def stream_current_telemetry_websocket(websocket: WebSocket):
    """Envia a amostra atual ao cliente a cada segundo até a desconexão."""
    await websocket.accept()
    try:
        while True:
            current = await asyncio.to_thread(_read_websocket_snapshot)
            await websocket.send_json(jsonable_encoder(current))
            await asyncio.sleep(settings.TELEMETRY_UPDATE_INTERVAL_MS / 1000)
    except WebSocketDisconnect:
        pass
