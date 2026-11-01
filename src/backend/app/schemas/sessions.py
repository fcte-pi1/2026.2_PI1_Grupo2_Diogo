from typing import Literal
from datetime import datetime
from pydantic import BaseModel

from pydantic import BaseModel


class StartExecutionInput(BaseModel):
    tipo: Literal["4x4", "8x4", "12x4"] = "4x4"


class FinishExecutionInput(BaseModel):
    status: Literal["concluido", "interrompido"] = "concluido"
    resultado: Literal["sucesso", "falha"] | None = None

class HistoryItem(BaseModel):
    """Um item da lista de histórico de execução."""

    id: str
    labirinto_id: str
    labirinto_nome: str
    labirinto_tipo: str

    iniciada_em: datetime
    finalizada_em: datetime | None
    tempo_total_ms: int | None
    status: str     # em_execucao | concluido | interrompido
    resultado: str | None   # sucesso | falha
    algoritmo: str
    velocidade_media_cm_s: float | None
    rpm: float | None
    bateria_consumida_pct: float | None
    quantidade_celulas_visitadas: int | None

class HistoryResponse(BaseModel):
    """Resposta em página do histórico."""

    total: int
    limit: int
    offset: int
    items: list[HistoryItem]
    
