import asyncio

from fastapi import APIRouter, Depends, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.encoders import jsonable_encoder
from pydantic import TypeAdapter, ValidationError
from sqlalchemy import text
from sqlalchemy.orm import Session

from app.core.config import settings
from app.core.database import SessionLocal, get_db
from app.schemas.telemetry import EspTelemetryInput, TelemetryData
from app.services.telemetry_service import telemetry_service
from app.services.esp_websocket import esp_websocket_manager


router = APIRouter(
    prefix="/telemetry",
    tags=["Telemetry"],
)
websocket_router = APIRouter(tags=["Telemetry"])
telemetry_input_adapter = TypeAdapter(TelemetryData | EspTelemetryInput)
CURRENT_TELEMETRY_INTERVAL_SECONDS = 1


@router.get("/status")
def get_esp_connection_status():
    """Estado de conexão inferido pelo recebimento recente de pacotes da ESP."""
    return telemetry_service.get_connection_status()

@router.get(
    "/current",
)
def get_current_telemetry(db: Session = Depends(get_db)):
    """Busca no banco a última amostra recebida, priorizando execução ativa."""
    return _get_current_telemetry(db)


def _get_current_telemetry(db: Session):
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

    if sample is None:
        raise HTTPException(
            status_code=503,
            detail="Nenhuma telemetria salva no banco.",
        )

    connection = telemetry_service.get_connection_status()
    return {
        "execucao_id": sample["execucao_id"],
        "telemetry_sample_id": sample["telemetry_sample_id"],
        "tempo_relativo_ms": sample["tempo_relativo_ms"],
        "position": {"row": sample["pos_y"], "col": sample["pos_x"]},
        "metrics": {
            "speed_cm_s": sample["velocidade_instantanea_cm_s"],
            "rpm": (sample["rpm_motor_esq"] + sample["rpm_motor_dir"]) / 2,
            "rpmLeft": sample["rpm_motor_esq"],
            "rpmRight": sample["rpm_motor_dir"],
            "battery_pct": sample["nivel_bateria_pct"],
        },
        "movement": {
            "giro_x_deg_s": sample["giro_x_deg_s"],
            "giro_y_deg_s": sample["giro_y_deg_s"],
            "giro_z_deg_s": sample["giro_z_deg_s"],
        },
        "events": [],
        "generated_at": sample["generated_at"],
        "served_at": connection["last_seen"],
        "age_ms": connection["age_ms"],
        "stale": not connection["connected"],
        "connected": connection["connected"],
    }


@router.post("")
def receive_telemetry(
    telemetry: EspTelemetryInput | TelemetryData,
    db: Session = Depends(get_db),
):
    """Recebe uma amostra da ESP e salva na execução ativa."""
    return _save_telemetry(telemetry, db)


def _save_telemetry(telemetry: EspTelemetryInput | TelemetryData, db: Session):
    reported_connected = (
        telemetry.connected if isinstance(telemetry, EspTelemetryInput) else True
    )
    telemetry_service.mark_esp_packet_received(reported_connected)

    execution = db.execute(
        text(
            "SELECT id, FLOOR(EXTRACT(EPOCH FROM "
            "(LOCALTIMESTAMP - iniciada_em)) * 1000)::integer AS elapsed_ms "
            "FROM execucao "
            "WHERE status = 'em_execucao' ORDER BY iniciada_em DESC LIMIT 1"
        )
    ).mappings().first()

    if execution is None:
        raise HTTPException(
            status_code=409,
            detail="Não existe execução ativa para associar a telemetria.",
        )

    elapsed_ms = max(0, execution["elapsed_ms"])
    if isinstance(telemetry, TelemetryData):
        battery = telemetry.metrics.battery_pct
        speed = telemetry.metrics.speed_cm_s
        rpm_left = int(
            telemetry.metrics.rpm_left
            if telemetry.metrics.rpm_left is not None
            else telemetry.metrics.rpm
        )
        rpm_right = int(
            telemetry.metrics.rpm_right
            if telemetry.metrics.rpm_right is not None
            else telemetry.metrics.rpm
        )
        pos_x = telemetry.position.col
        pos_y = telemetry.position.row
    else:
        battery = telemetry.battery_pct
        speed = telemetry.speed_cm_s
        rpm_left = telemetry.rpm
        rpm_right = telemetry.rpm
        position = telemetry.position
        pos_x = position.col if position is not None else telemetry.col
        pos_y = position.row if position is not None else telemetry.row

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
            "SELECT pos_x, pos_y FROM amostra_movimento "
            "WHERE execucao_id = :execucao_id ORDER BY id DESC LIMIT 1"
        ),
        {"execucao_id": execution["id"]},
    ).mappings().first()

    # Pacotes antigos podem não carregar posição; nesse caso, mantenha a última
    # posição registrada em vez de gravar um retorno artificial para (0, 0).
    if pos_x is None or pos_y is None:
        pos_x = last_movement["pos_x"] if last_movement is not None else 0
        pos_y = last_movement["pos_y"] if last_movement is not None else 0

    if last_telemetry is not None and last_movement is not None:
        unchanged = (
            round(last_telemetry["nivel_bateria_pct"], 1) == round(battery, 1)
            and round(last_telemetry["velocidade_instantanea_cm_s"], 2) == round(speed, 2)
            and last_telemetry["rpm_motor_esq"] == rpm_left
            and last_telemetry["rpm_motor_dir"] == rpm_right
            and last_movement["pos_x"] == pos_x
            and last_movement["pos_y"] == pos_y
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
            ") VALUES (:execucao_id, :tempo_relativo_ms, 0, 0, 0, :pos_x, :pos_y) "
            "RETURNING id"
        ),
        {
            "execucao_id": execution["id"],
            "tempo_relativo_ms": elapsed_ms,
            "pos_x": pos_x,
            "pos_y": pos_y,
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
    telemetry_service.mark_websocket_connected()
    try:
        while True:
            try:
                payload = await websocket.receive_json()
                telemetry = telemetry_input_adapter.validate_python(payload)
            except (ValidationError, ValueError, TypeError) as error:
                await esp_websocket_manager.send_json(
                    {"status": "error", "detail": str(error)}
                )
                continue

            db = SessionLocal()
            try:
                result = _save_telemetry(telemetry, db)
                await esp_websocket_manager.send_json(result)
            except HTTPException as error:
                db.rollback()
                await esp_websocket_manager.send_json(
                    {"status": "error", "detail": error.detail, "code": error.status_code}
                )
            except Exception:
                db.rollback()
                await esp_websocket_manager.send_json(
                    {"status": "error", "detail": "Falha ao salvar telemetria."}
                )
                raise
            finally:
                db.close()
    except WebSocketDisconnect:
        pass
    finally:
        esp_websocket_manager.disconnect(websocket)
        telemetry_service.mark_websocket_disconnected()


@websocket_router.websocket("/ws/telemetry/current")
async def stream_current_telemetry_websocket(websocket: WebSocket):
    """Envia a amostra atual ao cliente a cada segundo até a desconexão."""
    await websocket.accept()
    try:
        while True:
            db = SessionLocal()
            try:
                current = _get_current_telemetry(db)
            except HTTPException as error:
                current = {"status": "error", "detail": error.detail, "code": error.status_code}
            finally:
                db.close()
            await websocket.send_json(jsonable_encoder(current))
            await asyncio.sleep(CURRENT_TELEMETRY_INTERVAL_SECONDS)
    except WebSocketDisconnect:
        pass
