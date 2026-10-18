from fastapi import FastAPI

from app.core.database import test_database_connection


app = FastAPI(
    title="Micromouse API",
    version="0.1.0",
)


@app.get("/")
def root():
    return {
        "message": "Micromouse API funcionando"
    }


@app.get("/health")
def health():
    try:
        test_database_connection()

        return {
            "status": "ok",
            "database": "connected",
        }

    except Exception as error:
        return {
            "status": "error",
            "database": "disconnected",
            "detail": str(error),
        }


# ==============================================================================
# PROPOSTA DE INTEGRAÇÃO - EQUIPE DE ELETRÔNICA / FIRMWARE (ISSUES 66 e 120)
# ==============================================================================
# Olá equipe de Software!
# Para que o robô (ESP32) possa enviar os dados de telemetria em tempo real via
# Wi-Fi e medir a latência da comunicação (RTT) sem quebrar nada do que vocês
# estão desenvolvendo, deixamos abaixo o modelo de dados (Pydantic) e os endpoints
# sugeridos comentados.
#
# Como ativar/utilizar:
# 1. Quando forem efetivar a integração, basta descomentar este bloco (ou mover
#    para um roteador dedicado, ex.: app/routers/telemetria.py).
# 2. O payload está 100% alinhado com a tabela 'amostra_telemetria' (db/01-schema.sql)
#    e com o frontend (types/telemetry.ts).
# 3. Fiquem à vontade para continuar
#    no PostgreSQL com o SQLAlchemy ou
#    migrar para WebSockets, se preferirem.
# ==============================================================================

# import time
# from typing import Optional
# from fastapi.middleware.cors import CORSMiddleware
# from pydantic import BaseModel
#
# # Habilita CORS para o frontend React
# app.add_middleware(
#     CORSMiddleware,
#     allow_origins=["*"],
#     allow_credentials=True,
#     allow_methods=["*"],
#     allow_headers=["*"],
# )
#
# # Modelos de dados para validação do payload de telemetria recebido do ESP32
# class BatteryModel(BaseModel):
#     nivel_pct: float
#     tensao_v: float
#     consumo_ma: float
#
# class MotorsModel(BaseModel):
#     rpm_esq: int
#     rpm_dir: int
#     velocidade_cm_s: float
#
# class ImuModel(BaseModel):
#     yaw_deg: float
#     taxa_deg_s: float
#     giro_x: float
#     giro_y: float
#     giro_z: float
#
# class DistanceSensorsModel(BaseModel):
#     esq: int
#     frente: int
#     dir: int
#
# class PositionModel(BaseModel):
#     x: int
#     y: int
#     direcao: str = "N"
#
# class DiagnosticModel(BaseModel):
#     latencia_ms: int = 0
#     pacotes_enviados: int = 0
#     pacotes_falhos: int = 0
#     conectado: bool = True
#
# class TelemetryPayload(BaseModel):
#     execucao_id: int = 1
#     tempo_relativo_ms: int
#     timestamp_envio_ms: int
#     bateria: BatteryModel
#     motores: MotorsModel
#     imu: ImuModel
#     sensores_distancia_mm: DistanceSensorsModel
#     posicao_estimada: PositionModel
#     status_missao: str = "em_execucao"
#     diagnostico: Optional[DiagnosticModel] = None
#
# # Cache em memória para os testes de bancada do robô
# latest_telemetry: Optional[dict] = None
# latest_telemetry_timestamp: float = 0.0
#
# @app.post("/api/telemetria")
# def receive_telemetry(payload: TelemetryPayload):
#     """
#     Endpoint consumido pelo firmware do ESP32 para recepção contínua de telemetria via Wi-Fi.
#     Responde com timestamp de recepção do servidor para permitir medição de latência RTT.
#     """
#     global latest_telemetry, latest_telemetry_timestamp
#     server_time_ms = int(time.time() * 1000)
#
#     data = payload.model_dump()
#     data["server_received_at_ms"] = server_time_ms
#     latest_telemetry = data
#     latest_telemetry_timestamp = time.time()
#
#     return {
#         "status": "ok",
#         "execucao_id": payload.execucao_id,
#         "server_time_ms": server_time_ms,
#     }
#
# @app.get("/api/telemetria/atual")
# def get_current_telemetry():
#     """
#     Retorna o snapshot mais recente da telemetria, formatado para consumo direto
#     pelos componentes da aplicação web (React).
#     """
#     global latest_telemetry, latest_telemetry_timestamp
#
#     agora = time.time()
#     conectado = (latest_telemetry is not None) and ((agora - latest_telemetry_timestamp) < 3.0)
#
#     if not latest_telemetry or not conectado:
#         return {
#             "connected": False,
#             "status": "Sem conexão",
#             "battery": {
#                 "percentage": 0,
#                 "voltage": 0.0,
#                 "current": 0.0,
#                 "estimatedRechargeMinutes": 0,
#             },
#             "averageSpeed": 0.0,
#             "motors": {"leftRpm": 0, "rightRpm": 0},
#             "gyro": {"x": 0.0, "y": 0.0, "z": 0.0},
#             "posicao": {"x": 0, "y": 0, "direcao": "N"},
#             "tempoMS": 0,
#             "distancias_mm": {"esq": -1, "frente": -1, "dir": -1},
#             "latencia_ms": 0,
#         }
#
#     raw = latest_telemetry
#     bat = raw.get("bateria", {})
#     mot = raw.get("motores", {})
#     imu = raw.get("imu", {})
#     pos = raw.get("posicao_estimada", {})
#     diag = raw.get("diagnostico") or {}
#     pct = bat.get("nivel_pct", 0)
#
#     return {
#         "connected": True,
#         "status": raw.get("status_missao", "em_execucao"),
#         "battery": {
#             "percentage": pct,
#             "voltage": bat.get("tensao_v", 0.0),
#             "current": bat.get("consumo_ma", 0.0) / 1000.0,
#             "estimatedRechargeMinutes": int(max(0, (100.0 - pct) * 0.8)),
#         },
#         "averageSpeed": mot.get("velocidade_cm_s", 0.0) / 100.0,
#         "motors": {
#             "leftRpm": mot.get("rpm_esq", 0),
#             "rightRpm": mot.get("rpm_dir", 0),
#         },
#         "gyro": {
#             "x": imu.get("giro_x", 0.0),
#             "y": imu.get("giro_y", 0.0),
#             "z": imu.get("yaw_deg", 0.0),
#         },
#         "posicao": {
#             "x": pos.get("x", 0),
#             "y": pos.get("y", 0),
#             "direcao": pos.get("direcao", "N"),
#         },
#         "tempoMS": raw.get("tempo_relativo_ms", 0),
#         "distancias_mm": raw.get("sensores_distancia_mm", {}),
#         "latencia_ms": diag.get("latencia_ms", 0),
#         "pacotes_enviados": diag.get("pacotes_enviados", 0),
#         "pacotes_falhos": diag.get("pacotes_falhos", 0),
#     }