# API da tela Labirinto

API REST em [FastAPI](https://fastapi.tiangolo.com/) que entrega os dados da tela **Labirinto** do frontend: a execução (corrida) ativa, o labirinto, a trajetória do robô, as métricas e o log de eventos.

Por enquanto os dados são **simulados (mock)** e ficam na memória do servidor. Ainda não há integração com o robô nem com o banco de dados. O formato das respostas já é o definitivo, para o frontend não precisar mudar quando os dados reais chegarem (issue #111).

## Como funciona

```
navegador ──► frontend (:5173) ──proxy /api──► backend (:8000)
                                                │
                                   app/routes/sessions.py   as rotas
                                                │ leem
                                   app/mock (state)         dados em memória,
                                                            montados quando o servidor liga
```

1. Ao ligar, o backend monta uma execução de exemplo (`0847`, algoritmo Flood Fill) num labirinto 4×4 fixo. É o *seed*.
2. Cada rota lê esse estado e devolve JSON. Os modelos de `app/models` garantem o formato.
3. O frontend pergunta a cada 1 s (*polling*) e redesenha a tela.

## Estrutura

Os caminhos são a partir de `src/backend`.

| Caminho | O que tem |
|---|---|
| `app/main.py` | Cria o app FastAPI e registra as rotas com o prefixo `/api`. |
| `app/routes/sessions.py` | As rotas da tela Labirinto e a checagem da execução ativa (404). |
| `app/models/` | O formato dos dados (Pydantic): execução, labirinto, trajetória, métricas e eventos. Ver [o README dos modelos](../models/README.md). |
| `app/mock/` | Os dados simulados (seed e labirinto) e a simulação do robô. Ver [o README do mock](../mock/README.md). |
| `tests/` | Testes automáticos (pytest). Ver [o README dos testes](../../tests/README.md). |

## Rotas

Todas começam com `/api`. Com o servidor rodando, a documentação interativa fica em http://127.0.0.1:8000/docs, e dá para testar cada rota pelo botão *Try it out*.

Os caminhos das rotas continuam com `sessions`, mas o que elas devolvem segue o modelo **Execução** da documentação do projeto (`app/models/execution.py`).

| Rota | Devolve | Se a execução não existir |
|---|---|---|
| `GET /api/sessions/active` | A execução ativa, ou `null` se não houver. Os campos são `id`, `labirinto_id`, `status`, `algorithm`, `tempo_total_ms`, `iniciada_em`, `finalizada_em`, `resultado`, `velocidade_media_cm_s` e `criado_em`. | — |
| `GET /api/sessions/{id}/maze` | O labirinto: `size`, `grid[row][col]` com as paredes de cada célula (`parede_norte`, `parede_sul`, `parede_leste`, `parede_oeste`), `start` e `goal`. | 404 |
| `GET /api/sessions/{id}/path` | A trajetória: lista de `{row, col}`, do início até a posição atual. | 404 |
| `GET /api/sessions/{id}/metrics` | As métricas atuais: `speed_cm_s`, `rpm` e `battery_pct`. | 404 |
| `GET /api/sessions/{id}/events` | O log de eventos: `timestamp`, `type` e `message`. | 404 |

O campo `status` vale `em_execucao`, `concluido` ou `interrompido`, como na documentação do projeto. O tempo (`tempo_total_ms`) é em **milissegundos**.

- **Como o front usa:** primeiro ele pergunta o `/active` para descobrir o `id` da execução, e depois usa esse id nas outras rotas. Nenhum id fica fixo no front.
- **Por que o `/active` devolve `null` e não 404:** não ter execução não é erro, é só o robô desligado (o front mostra "Sem conexão"). Já pedir uma execução que não existe é erro de quem pediu, por isso as rotas com `{id}` respondem `404 {"detail": "No active session found"}`.

## Como rodar

Os comandos são para o PowerShell (Windows), dentro de `src/backend`. No Linux ou no Mac, troque `.venv\Scripts\python` por `.venv/bin/python`.

### Preparar o ambiente (só na primeira vez)

```powershell
python -m venv .venv
.venv\Scripts\python -m pip install fastapi uvicorn pytest httpx
```

A `.venv` é uma pasta com um Python só deste projeto. Ela está no `.gitignore` e não vai para o repositório. Ainda não há `requirements.txt`; os pacotes acima são os necessários.

### Operação normal: dados do mock, parados

```powershell
.venv\Scripts\python -m uvicorn app.main:app --reload --host 0.0.0.0
```

- `--reload` reinicia o servidor sozinho quando você salva um arquivo.
- `--host 0.0.0.0` faz o backend aceitar conexões de fora do próprio PC. É obrigatório quando o frontend roda no Docker, porque para o container o seu PC é "outro computador".

A tela mostra a execução 0847 já concluída: status *Concluído*, 00:42, 7 células, posição (3, 3), 12,5 cm/s, 310 rpm e bateria 87%.

### Com a simulação: robô andando

```powershell
.venv\Scripts\python -m uvicorn app.mock.dev_server:app --reload --host 0.0.0.0
```

O robô começa a andar quando o servidor liga: uma célula por segundo, até chegar na meta em 6 s. Para ver de novo, reinicie o servidor. Com o `--reload`, salvar qualquer arquivo também reinicia. Como funciona: [o README do mock](../mock/README.md).

### Junto com o frontend

1. Suba o backend com um dos comandos acima.
2. Suba o frontend com `docker compose up -d` em `src/frontend/web-micromouse` (detalhes no README de lá).
3. Abra http://localhost:5173/labirinto.

## Testes

```powershell
.venv\Scripts\python -m pytest -v
```

Use `python -m pytest`, e não só `pytest`. O `-m` coloca a pasta atual no caminho de import, e é isso que faz o `from app...` dos testes funcionar.

| Arquivo | O que testa |
|---|---|
| `tests/test_seed.py` | O labirinto e a trajetória do seed: tamanho, bordas fechadas, paredes simétricas e um caminho que não atravessa parede. |
| `tests/test_routes.py` | `GET /api/sessions/active`, com e sem execução. |
| `tests/test_execution_data.py` | `/maze`, `/path`, `/metrics` e `/events`, e o 404 de todas elas, tanto para id desconhecido quanto para quando não há execução. |
| `tests/test_database.py` | O link com o PostgreSQL: o `/health` responde `connected` e o que o `app/mock/seed_db.py` gravou bate com o mock (trajetória e paredes). Precisa do banco no ar: com a porta fechada é pulado; com a senha errada, falha. |

- Os testes de 404 usam `@pytest.mark.parametrize`: o mesmo teste roda uma vez para cada rota da lista. Se surgir uma rota nova com `{id}`, é só incluir o nome dela na lista.
- Os testes não carregam a simulação. Por isso os dados não mudam com o tempo e o resultado é sempre o mesmo.

## Decisões e por quês

- **Uma função só para o 404** (`check_active_session`, em `sessions.py`): a mesma regra vale para todas as rotas com `{id}`, então mudar a regra ou a mensagem é em um lugar só. O `/maze`, feito depois, reaproveitou a função.
- **404 documentado no `/docs`** (o `responses=` em cada rota): quem for usar a API vê que o 404 existe sem precisar ler o código.
- **O id é texto** (`"0847"`): como número ele viraria `847`, perdendo o zero, e não bateria com o da execução.
- **Os testes comparam com o próprio estado** (`len(state.path)`, `EXECUTION_ID`) em vez de números fixos: quando o seed mudou de 16×16 para 4×4, esses testes só precisaram trocar nomes (`SESSION_ID` → `EXECUTION_ID`), nenhum número.
- **Os nomes seguem a documentação do projeto:** a execução tem os campos da tabela Execução, os status são *em execução*, *concluído* e *interrompido*, e as paredes são norte, sul, leste e oeste.
- **A simulação fica isolada em `app/mock`:** só o `dev_server` carrega ela. O servidor de produção (`app.main`) nem sabe que ela existe.

## Quem fez o quê (issue #111)

- **João (@Karmantinedev):**
  - os modelos de dados, incluindo a reestruturação para o modelo Execução da documentação e os nomes das paredes;
  - o seed e o labirinto, hoje 4×4;
  - o `GET /active` e o `GET /maze`.
- **Cauã (@CauaHenriqueM):**
  - o `GET /path`, o `/metrics` e o `/events`;
  - o 404 (`check_active_session` e a documentação no `/docs`);
  - os testes de `tests/test_execution_data.py`;
  - a simulação (`app/mock/simulation.py` e `app/mock/dev_server.py`);
  - a integração com o frontend.

## Pendências conhecidas

- **Labirintos retangulares (8×4 e 12×4):** o modelo `Maze` guarda um número só (`size`), então só representa labirintos quadrados. Para 8×4 e 12×4, ele vai precisar de linhas e colunas separadas. Quando isso mudar, o front também precisa mudar, em `src/api.ts` (tipo `ApiMaze`) e em `src/hooks/useLabirinto.ts` (`linhas` e `colunas`).
- **Banco de dados (PostgreSQL):** é a próxima etapa. Hoje os dados somem quando o servidor reinicia.
- **`requirements.txt`:** ainda não existe.
