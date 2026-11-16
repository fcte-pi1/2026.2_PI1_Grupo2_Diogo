# Alteracoes sobre a V4.4.2

- Port de `motion_control` RETO de branch testes-de-encoders: PI e sincronismo suave + odometria de diagnostico.
- Port seletivo de `web_telemetry` RETO e U3 da branch, com RESTAURACAO de `prepararMotoresControle()` e status STBY/driver da V4.4.2.
- Port seletivo `atualizarLaserFrontal()` nao bloqueante, apenas durante teste armado.
- Novo painel para reto e U3, mantendo abas originais de encoders, giro e lasers.
- Integracao INA219 sem duplicidade, dados filtrados de V/I/potencia, SEM corte automatico por padrao.
- HTTP backend em FreeRTOS Core 0 OPCIONAL; sem network no `loop` do PI.
- Mapa 16x16 em RAM, sem gerar pose ficticia; interface futura para navegacao.
- Mantidos canais motores A=esquerdo/B=direito, polaridades atuais, XSHUT 18/19/23 e STBY LOW na parada.
