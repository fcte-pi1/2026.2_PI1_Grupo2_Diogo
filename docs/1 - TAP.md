# Termo de Abertura do Projeto

## Visão Geral do Projeto

### Dados do projeto

- **Nome do Projeto:** Competição Micromouse
- **Data de Início:** 01/09/2026
- **Data de Término:** 18/12/2026
- **Patrocinador:** Universidade de Brasília


### Objetivos

- **Specific**: Desenvolver um robô autônomo (Micromouse) com dimensões máximas de $16,5 cm \times 16,5 cm$, capaz de mapear paredes, monitorar sua localização em tempo real e resolver $3$ configurações progressivas de labirintos desconhecidos ($4\times4$ com $72\times 72 cm^2$, $8\times4$ com $144\times72 cm^2$ e $12\times4$ com $216\times72 cm^2$) partindo de um canto e alcançando o objetivo no canto diametralmente oposto, além de construir um sistema web com banco de dados para recepção, exibição de telemetria em tempo real e consulta pós-desafio.
- **Measurable**: 
    - Cumprimento de cada labirinto dentro do limite máximo de 10 minutos por desafio.

    - Conclusão do trajeto preferencialmente na 1ª tentativa (nota máxima 10,0 por labirinto).

    - Exibição de $100\%$ dos dados de telemetria no sistema web durante as corridas (evitando o fator redutor de $0,75$ da avaliação).

    - Construção e validação prévia de uma pista de testes simplificada $4 \times 4$.
- **Agreed**: Alinhado entre a equipe multidisciplinar de estudantes e os professores responsáveis pela disciplina de PI1 (**Profs. Diogo C. Garcia, Juliana P. Rodrigues, Lui T. C. Habl, Bruno L. Pereira e Hilmer Rodrigues Neri**).

- **Realistic**: Projeto concebido do zero integrando os conhecimentos das engenharias da FCTE (sem uso de soluções comerciais prontas de Micromouse), com cronograma estruturado em testes modulares (*AP12*) e testes de integração prévios (*AP18*) para mitigação de falhas e bugs antes da entrega final.
- **Time Bound**: Finalização e homologação de todas as entregas dentro das $16$ semanas do semestre letivo $2026.2$, cumprindo os marcos de *AP2* a *AP20*.  

### Público-Alvo

- Público Direto: Corpo docente e banca avaliadora da disciplina de Projeto Integrador 1 (PI1) da FCTE/UnB. 
- Público Indireto: Estudantes e equipes de robótica da UnB/FCTE interessados em reaproveitamento de código aberto, esquemáticos eletrônicos, modelos CAD e telemetria para competições acadêmicas de Micromouse.  

### Descrição do Problema

A competição Micromouse consiste em resolver labirintos desconhecidos de forma $100\%$ autônoma, sem nenhuma intervenção humana, alteração de código ou modificação de memória durante o trajeto. No contexto de *PI1* ($2026.2$), o robô deve navegar por células de $18 cm$ de lado com paredes brancas de $5 cm$ de altura e topo vermelho sobre chão preto, operando dentro do limite físico de $16,5 cm$ de largura/comprimento, em três configurações de pista distintas ($4\times4$, $8\times4$ e $12\times4$).  Além do desafio de sensoriamento, controle e algoritmo embarcado, o projeto exige a integração com um sistema web completo que receba e exiba dados telemétricos ao vivo (trajeto, bateria, velocidade média, tempo e cumprimento do objetivo) e armazene os dados em um banco para consultas posteriores por labirinto específico ou visão geral. O projeto se justifica pela concepção autoral multidisciplinar, mitigando riscos de falhas por meio de metodologias ágeis e testes de integração antecipados.

### Indicadores

- Número de estudantes matriculados anualmente nas cinco engenharias da FCTE/UnB;
- Número de turmas e grupos cursando as disciplinas de Projeto Integrador (PI1 e PI2) por semestre;
- Número de equipes de competição acadêmica de robótica e automobilismo ativas na UnB;
- Número de instituições de ensino superior (públicas e privadas) no Distrito Federal que ofertam cursos de Engenharia e Computação;
- Número de laboratórios de prototipagem rápida e Makerspaces acadêmicos no DF;
- Número de escolas de ensino médio e técnico profissionalizante (ex: IFB, SENAI-DF) no DF;
- Número de empresas de automação, logística interna (AGVs) e tecnologia sediadas no Polo de Tecnologia do DF.

### Membros da Equipe

| **Nome** | **Matrícula** | **Curso** | **E-mail** | **Funções** |
|:------------------|:-----------:|:-----------------|:---------------------------|:---------------------------|
| Yan Santos Rodrigues | 241025480 | Engenharia de Software | yansantosrodrigues.dev@gmail.com | Gerente Geral do Projeto |
| Pedro Henrique Raposo Lima | 251040416 | Engenharia Aeroespacial | raposolima.pedro@gmail.com | Sub-gerente de Estruturas |
| Rian Ferreira Alencar | 251027817 | Engenharia Eletrônica | rianalencar6@gmail.com | Sub-gerente de Eletrônica |
| Daniel Fernandes Silva | 222008459 | Engenharia de Software | danielaulounb@gmail.com | Sub-gerente de Software |
| João Pedro Ferreira | 211061940 | Engenharia de Software | jp1792464@gmail.com | Sub-gerente de Energia |
| Pedro Henrique Inácio dos Santos | 241026001 | Engenharia de Software| pedrohenriquesantosinacio@gmail.com | Equipe de Estruturas |
| João Pedro Rodrigues Duarte | 241025668 | Engenharia Aeroespacial | joaopedroduarte203@gmail.com | Equipe de Estruturas |
| Rodrigo Átila Tavares de Oliveira | 241025855 | Engenharia de Software | 241025855@aluno.unb.br | Equipe de Estruturas |
| Ângelo Araujo Cordova | 241025917 | Engenharia de Software | angeloaraujocordova@gmail.com | Equipe de Eletrônica |
| Pedro Gomes Oliveira | 211031440 | Engenharia de Software | pedro.oliveira1@outlook.com | Equipe de Eletrônica |
| Ricardo Lucas Winchello Vieira Branco | 221008409 | Engenharia de Software | ricardolucasb@gmail.com | Equipe de Eletrônica |
| Gabriel Souza de Matos | 251009229 | Engenharia de Software | gsmcttunb@gmail.com | Equipe de Software |
| João Vitor Justo Gonçalves | 241012267 | Engenharia de Software | joaovitorjusto3@gmail.com | Equipe de Software |
| João Pedro Gonzaga dos Santos Souza | 231011551 | Engenharia de Software | 231011551@aluno.unb.br | Equipe de Software |
| Cauã Henrique Moura Rodrigues | 231026661 | Engenharia de Software | cauahmourarodrigues@gmail.com | Equipe de Software |
| Eduardo Silva Waski | 231011284 | Engenharia de Software | eduardowaski688@gmail.com | Equipe de Energia |
| João Pedro Rodrigues Gomes da Silva | 231026966 | Engenharia de Software | 231026966@aluno.unb.br | Equipe de Energia |
| Júlia Dos Reis Teixeira Massuda | 231035150 | Engenharia de Software | julia.massuda@gmail.com | Equipe de Energia |

**Orientador:** Diogo Caetano Garcia

### Orçamento estimado (R$)

O orçamento geral estimado do projeto no semestre será de **R$ 810,00**, ou seja, **R$ 45,00** por membro do grupo (18 membros).

### Duração estimada (horas)

- Carga horária semanal por membro: ~6 horas semanais (4 horas obrigatórias em aula, 2 aulas extra-classe, compreendendo ritos ágeis, desenvolvimento de código, montagem e testes em laboratório).
- Duração do semestre letivo: 16 semanas.
- Esforço estimado por membro: ~96 horas.
- Esforço total estimado da equipe (18 integrantes): ~1728 horas de trabalho.









