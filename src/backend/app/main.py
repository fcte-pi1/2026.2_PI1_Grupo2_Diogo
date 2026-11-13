from fastapi import FastAPI
from app.core.config import settings
from app.core.database import test_database_connection
from app.routes import sessions, telemetry
from fastapi.middleware.cors import CORSMiddleware


app = FastAPI(
    title="Micromouse API",
    version="0.1.0",
)

app.include_router(sessions.router, prefix="/api")
app.include_router(telemetry.router, prefix="/api")
app.include_router(telemetry.websocket_router)

app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_origins_list,
    # Vite may be opened through localhost or through this machine's LAN IP.
    # Accept those development origins in both npm and Docker workflows.
    allow_origin_regex=(
        r"^https?://(?:localhost|127\.0\.0\.1|0\.0\.0\.0|"
        r"192\.168\.\d{1,3}\.\d{1,3}|10\.\d{1,3}\.\d{1,3}\.\d{1,3}|"
        r"172\.(?:1[6-9]|2\d|3[01])\.\d{1,3}\.\d{1,3})(?::\d+)?$"
    ),
    allow_methods=["*"],
    allow_headers=["*"],
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
