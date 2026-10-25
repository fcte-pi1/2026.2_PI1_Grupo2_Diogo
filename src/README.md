# Micromouse — candidato à integração do giro V4.3

**Estado:** núcleo de controle validado experimentalmente em bancada (relato do usuário); **não validado para curvas com paredes próximas**. Estes são os arquivos originais da V4.3, copiados **sem alterar** as rotinas. Este pacote é uma **seleção de módulos, não um projeto PlatformIO completo**. Mescle com a branch de integração e **não sobrescreva o `config.h` existente sem conferir GPIOs e parâmetros**.

## Decisão técnica

- Usar o **sincronismo suave** (`sincronismoSuave = true`) como padrão.
- Começar com **encoder-only** (`usaImu = false`) para tomar a decisão de parada. A IMU pode ser usada em modo diagnóstico, quando estiver confiável.
- Manter o giro IMU como modo opcional, sujeito a calibração de bias e confirmação angular externa.
- Preservar **35 RPM** como limite inicial (aumentar apenas após ensaios com paredes).
- Não adicionar translação fixa automática para frente ou para trás; a **posição de início do giro pertence ao planejador de navegação**, com verificação de espaço livre.
- **Não habilitar giro autônomo junto a paredes** sem ensaio geométrico específico da interseção.

## Dependências

O `motion_control.cpp` necessita `config.h`, `motors.h`, `encoders.h`, `imu.h`. O driver dos encoders usa a biblioteca `madhephaestus/ESP32Encoder`. `imu.cpp` utiliza o I²C via `Wire` e aceita MPU-6050 (WHO_AM_I `0x68`) e o sensor identificado no protótipo como `0x74` (MPU-6515). Verificar dependências reais no PlatformIO da branch de integração.

Os arquivos incluídos em `firmware/` são os originais da bancada V4.3. O `config_REFERENCIA_NAO_SOBRESCREVER.h` é uma cópia de referência, NÃO deve coexistir como `config.h` sem conferência das definições.

## Fluxo mínimo de integração

No setup da aplicação, com as rotinas adequadas:

```cpp
setupMotors();
Wire.begin(PIN_SDA, PIN_SCL);
Wire.setClock(100000);
iniciarIMU();
if (!iniciarEncoders()) {
    frearMotoresTudo();
    // entrar em estado seguro
}
```

Os sensores VL53L0X compartilham o I²C: no firmware definitivo, inicialize/reenderece os sensores com XSHUT um a um antes de começar a navegação. Não copie o bloqueio por XSHUT da bancada como configuração permanente.

Para começar um giro à direita de 90° **usando somente encoders para controle**:

```cpp
bool ok = iniciarControleGiro(
    true,       // direita
    false,      // usaImu: false = somente diagnostico angular
    35.0f,     // limite de RPM inicial
    cprEsq,     // CPR medido ou lido da memoria
    cprDir,
    true        // sincronismo suave
);
if (!ok) {
    // não iniciar navegação: controlador ocupado / dados invalidos
}
```

No laço principal, **não bloqueante**, e com apenas um consumidor chamando `atualizarControleMovimento()`:

```cpp
atualizarIMU();
atualizarEncoders();
const char* motivo = atualizarControleMovimento();
if (motivo != nullptr) {
    ControleInfo finalInfo = lerControleMovimento();
    frearMotoresTudo();  // sempre frear antes da troca de estado
    desativarControleMovimento();
    const bool sucesso = strcmp(motivo, "giro_encoder_concluido") == 0 ||
                          strcmp(motivo, "giro_imu_concluido") == 0;
    // registrar motivo e finalInfo; avisar o planejador e
    // impedir movimento seguinte em caso de erro.
}
```

Use `#include <string.h>` para `strcmp`. Este trecho é um **exemplo de integração**: o controlador de navegação deve garantir um único comando ativo, timeout e parada por obstáculo. **O exemplo não checa paredes**.

## Geometria e risco de colisão

Medidas relatadas do chassi: frente do eixo 85 mm, atrás 30 mm, largura 120 mm. O centro da carroceria fica **27,5 mm à frente do eixo**, e um canto frontal fica a `sqrt(85²+60²) ≈ 104 mm` do eixo. Em uma célula com largura livre 168 mm, metade da largura é 84 mm e a margem lateral com um robô centralizado é `(168−120)/2 = 24 mm`.

Uma rotação pontual não garante que a carroceria se mantenha no centro da próxima passagem: a mudança de orientação desloca o centro da carroceria em 27,5 mm por componente quando a frente vira 90°. O risco é geométrico mesmo com encoders precisos. As folgas reais dependem da topologia da interseção, da posição no início da curva, das paredes adjacentes, cabos e tolerâncias mecânicas.

**Condições para liberar o giro em labirinto**

1. Medir a distância livre real entre paredes e o envelope completo do robô, incluindo fios/sensores.
2. Desenhar o envelope de varredura para curva à esquerda e à direita em uma interseção e em uma célula de fundo de corredor.
3. Definir a distância adequada antes da parede frontal e a posição do eixo onde a manobra começa. Os lasers devem participar do **posicionamento antes** da curva, não só da centralização depois.
4. Testar na pista com paredes reais (primeiro afastadas/protegidas) e medir folgas e deslocamento do eixo, com várias repetições.
5. Só então permitir o planejador pedir um giro em posição ocupada por paredes; senão, recusar a manobra ou utilizar trajetória planejada mais segura.

## Migração Git sugerida

Na branch de testes, guarde e marque o baseline V4.3. Na nova branch, partindo de `main`:

```bash
git switch main
git pull --ff-only
git switch -c feature/integracao-giro
```

Copie os arquivos selecionados para a estrutura de firmware do repositório, mescle os GPIOs/constantes, e compile com PlatformIO. **Não integre o `web_telemetry.cpp` de bancada**: mantenha apenas uma API de diagnóstico, se necessária.

```bash
git add src/firmware/
git commit -m "feat(firmware): integrar giro 90 graus com sincronismo suave"
```

Nenhuma alteração foi feita diretamente ao repositório do usuário; o pacote é local e serve para revisão antes da mesclagem.
