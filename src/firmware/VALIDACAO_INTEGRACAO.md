# Validação antes de considerar a integracao concluida

1. **Backup**: guarde o ZIP V4.4.2 funcional e a calibracao CPR em NVS. Este pacote NAO faz upload nem commit.
2. **Revisao eletrica**: GPIO27/BIN2 sem curto, GPIO5/STBY correto; nova pinagem no `config.h`.
3. **Build**: `pio run -e esp32dev`. Verifique uso de flash e RAM em output.
4. **Rodas suspensas**: PWM esquerdo/direito frente/re, parada segura e repeticao; CPR e sentido das contagens.
5. **Movimento/giro V4.3**: 90 graus a ambos sentidos, medir deslocamento final apos coast.
6. **Reto V4.5**: 50/100/150mm, sync ligado vs desligado; medir distancia real e diferencial entre rodas. Primeiro suspenso depois em pista livre.
7. **Laser U3**: reposicionamento exige medir o offset ate a face frontal; teste leitura valida/obsoleta com motores parados e ENTAO teste experimental frontal lento. Nunca considerar o teste como anticolisao validada.
8. **U2/U4**: filtros e capturas CSV; nao permitir correcoes por paredes ate reconstrucao dos suportes/calibracao.
9. **INA219**: dados observaveis de tensao, corrente e potencia; comparar com multimetro. Verificar margem de 2A da calibracao no hardware e bateria usada.
10. **WiFi**: painel local continua em 192.168.4.1; apos configurar backend externo e numero de celulas LiPo, testar HTTP 2Hz no Core 0 e observar heap, PI e interrupcoes com/sem rede.
11. **Mapa**: endpoint `/api/maze` deve iniciar com pose null e todas as paredes desconhecidas; nenhuma escrita automatica ate o modulo de navegacao da equipe chegar.
12. **Seguranca**: qualquer falha de heartbeat no TESTE motorizado cancela, mas nao dispensa chave fisica. Nao execute missao autonoma neste pacote.
