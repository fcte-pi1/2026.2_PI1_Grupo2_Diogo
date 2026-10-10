# Tests: o que está garantido

Os testes automáticos rodam com [pytest](https://docs.pytest.org/) e cobrem
**o seed e as rotas**. Não carregam a simulação: os dados ficam parados, então
o resultado é sempre o mesmo a cada execução.

## Como rodar

Os comandos são para o PowerShell (Windows), dentro de `src/backend`.

```powershell
.venv\Scripts\python -m pytest -v