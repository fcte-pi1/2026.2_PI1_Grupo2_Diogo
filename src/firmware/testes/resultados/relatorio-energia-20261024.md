
## Ensaio (24/10/2026 19:45)

Nenhum resultado salvo

| Medida | Valor |
|---|---|
| Duracao | 1.1 min (663 amostras) |
| Tensao | 7.46 V -> 7.46 V |
| Corrente media | 174.9 mA |
| Corrente maxima | 607.8 mA |
| Carga consumida | 3.2 mAh (0.2% de 1500 mAh) |
| Energia | 0.024 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 174.9 mA

## Ensaio (24/10/2026 19:51)

Nenhum resultado salvo

| Medida | Valor |
|---|---|
| Duracao | 0.4 min (233 amostras) |
| Tensao | 7.49 V -> 7.46 V |
| Corrente media | 172.3 mA |
| Corrente maxima | 607.8 mA |
| Carga consumida | 1.1 mAh (0.1% de 1500 mAh) |
| Energia | 0.008 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 172.8 mA

Eventos:
- 19:51:51 Comando desconhecido: [a[breltorio

## Ensaio (24/10/2026 19:52)


| Medida | Valor |
|---|---|
| Duracao | 0.3 min (190 amostras) |
| Tensao | 7.45 V -> 7.45 V |
| Corrente media | 173.6 mA |
| Corrente maxima | 607.8 mA |
| Carga consumida | 0.9 mAh (0.1% de 1500 mAh) |
| Energia | 0.007 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 174.2 mA

Eventos:
- 19:52:54 Consumo por componente armado: desligue, tire o USB, rodas no ar e ligue pela bateria

## Ensaio (24/10/2026 19:54)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.5 min (274 amostras) |
| Tensao | 7.41 V -> 7.45 V |
| Corrente media | 175.0 mA |
| Corrente maxima | 262.7 mA |
| Carga consumida | 1.3 mAh (0.1% de 1500 mAh) |
| Energia | 0.010 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 175.3 mA

## Ensaio (24/10/2026 19:59)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.2 min (132 amostras) |
| Tensao | 7.41 V -> 7.41 V |
| Corrente media | 173.3 mA |
| Corrente maxima | 267.7 mA |
| Carga consumida | 0.6 mAh (0.0% de 1500 mAh) |
| Energia | 0.005 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 173.5 mA

## Ensaio (24/10/2026 20:00)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.5 min (298 amostras) |
| Tensao | 7.42 V -> 7.40 V |
| Corrente media | 211.2 mA |
| Corrente maxima | 409.9 mA |
| Carga consumida | 1.7 mAh (0.1% de 1500 mAh) |
| Energia | 0.013 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 174.3 mA · 200 -> 234.1 mA

Eventos:
- 20:00:47 Motores em PWM 200
- 20:01:05 Motores parados
- 20:01:08 Comando desconhecido: relatorio[

## Ensaio (24/10/2026 20:03)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.5 min (303 amostras) |
| Tensao | 7.40 V -> 7.35 V |
| Corrente media | 284.7 mA |
| Corrente maxima | 515.2 mA |
| Carga consumida | 2.4 mAh (0.2% de 1500 mAh) |
| Energia | 0.018 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 174.5 mA · 200 -> 320.9 mA

Eventos:
- 20:03:10 Motores em PWM 200
- 20:03:33 Motores parados

## Ensaio (24/10/2026 20:05)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 1.1 min (672 amostras) |
| Tensao | 7.38 V -> 7.39 V |
| Corrente media | 210.7 mA |
| Corrente maxima | 988.1 mA |
| Carga consumida | 4.0 mAh (0.3% de 1500 mAh) |
| Energia | 0.029 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 173.8 mA · 255 -> 236.8 mA

Eventos:
- 20:05:22 Motores em PWM 255
- 20:05:25 Motores parados
- 20:05:27 Motores em PWM 255
- 20:05:29 Motores parados
- 20:05:31 Motores em PWM 255
- 20:05:33 Motores parados
- 20:05:34 Motores em PWM 255
- 20:05:37 Motores parados
- 20:05:38 Motores em PWM 255
- 20:05:41 Motores parados
- 20:05:43 Motores em PWM 255
- 20:05:46 Motores parados
- 20:05:47 Motores em PWM 255
- 20:05:50 Motores parados
- 20:05:51 Motores em PWM 255
- 20:05:54 Motores parados
- 20:05:55 Motores em PWM 255
- 20:05:58 Motores parados
- 20:05:59 Motores em PWM 255
- 20:06:01 Motores parados
- 20:06:03 Motores em PWM 255
- 20:06:05 Motores parados
- 20:06:06 Motores em PWM 255
- 20:06:09 Motores parados
- 20:06:11 Motores em PWM 255
- 20:06:13 Motores parados

## Ensaio (24/10/2026 20:08)


| Medida | Valor |
|---|---|
| Duracao | 2.0 min (1197 amostras) |
| Tensao | 7.33 V -> 7.38 V |
| Corrente media | 220.6 mA |
| Corrente maxima | 988.1 mA |
| Carga consumida | 7.3 mAh (0.5% de 1500 mAh) |
| Energia | 0.054 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 175.1 mA · 255 -> 235.4 mA

Eventos:
- 20:09:24 Motores em PWM 255
- 20:10:54 Motores parados

## Ensaio (24/10/2026 20:11)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.5 min (314 amostras) |
| Tensao | 7.33 V -> 7.37 V |
| Corrente media | 188.7 mA |
| Corrente maxima | 988.1 mA |
| Carga consumida | 1.6 mAh (0.1% de 1500 mAh) |
| Energia | 0.012 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 174.7 mA · 150 -> 219.4 mA

Eventos:
- 20:11:41 Motores em PWM 150
- 20:11:48 Corte em 8.00 V (padrao 6.6 V)
- 20:11:50 Bateria critica: 7.34 V, motores cortados
- 20:11:58 Corte em 6.60 V (padrao 6.6 V)

## Ensaio (24/10/2026 20:13)


| Medida | Valor |
|---|---|
| Duracao | 0.8 min (504 amostras) |
| Tensao | 7.34 V -> 7.37 V |
| Corrente media | 203.8 mA |
| Corrente maxima | 1041.6 mA |
| Carga consumida | 2.9 mAh (0.2% de 1500 mAh) |
| Energia | 0.021 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 175.3 mA · 255 -> 236.3 mA

Eventos:
- 20:14:06 Motores em PWM 255
- 20:14:12 Motores parados
- 20:14:23 Comando desconhecido: [a[amotores 255motores 255
- 20:14:24 Motores em PWM 255
- 20:14:26 Motores em PWM 255
- 20:14:29 Motores parados
- 20:14:30 Motores em PWM 255
- 20:14:33 Motores parados
- 20:14:34 Motores em PWM 255
- 20:14:38 Motores parados
- 20:14:39 Motores em PWM 255

## Ensaio (24/10/2026 20:15)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.8 min (486 amostras) |
| Tensao | 7.36 V -> 7.34 V |
| Corrente media | 195.5 mA |
| Corrente maxima | 1041.6 mA |
| Carga consumida | 2.6 mAh (0.2% de 1500 mAh) |
| Energia | 0.019 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 255 -> 234.9 mA · 0 -> 171.2 mA

Eventos:
- 20:15:49 Motores parados
- 20:15:59 Motores em PWM 255
- 20:16:01 Motores parados
- 20:16:03 Motores em PWM 255
- 20:16:04 Motores parados
- 20:16:05 Motores em PWM 255
- 20:16:06 Motores parados
- 20:16:07 Motores em PWM 255
- 20:16:08 Motores parados
- 20:16:09 Motores em PWM 255
- 20:16:10 Motores parados
- 20:16:11 Motores em PWM 255
- 20:16:13 Motores parados
- 20:16:14 Motores em PWM 255
- 20:16:15 Motores parados
- 20:16:16 Motores em PWM 255
- 20:16:17 Motores parados

## Ensaio (24/10/2026 20:19)


## Consumo por componente (modo automatico, sem USB)

| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |
|---|:---:|:---:|:---:|:---:|
| E0 base | 7.48 | 75.5 | 65.4 | 81.5 |
| E1 + MPU6050 | 7.48 | 80.6 | 75.9 | 82.1 |
| E2 + laser esquerdo | 7.47 | 89.9 | 76.7 | 97.4 |
| E3 + laser frontal | 7.45 | 99.8 | 75.4 | 114.0 |
| E4 + laser direito | 7.44 | 109.5 | 76.9 | 128.4 |
| E5 + WiFi ligado | 7.43 | 173.2 | 138.1 | 255.6 |
| E6 + WiFi transmitindo | 7.43 | 180.0 | 144.2 | 255.0 |
| E7 + motor A 50% | 7.43 | 200.2 | 166.0 | 281.8 |
| E8 motor A 100% | 7.42 | 210.2 | 165.3 | 290.8 |
| E9 + motor B 100% | 7.42 | 338.5 | 213.0 | 482.0 |

| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |
|---|:---:|:---:|:---:|:---:|
| ESP32 sem WiFi + reguladores + encoders + INA219 | 75.5 | - | - | - |
| MPU6050 | 5.1 | 9.3 | 3.3 V | 5 |
| VL53L0X esquerdo | 9.3 | 16.8 | 3.3 V | 20 |
| VL53L0X frontal | 9.9 | 17.9 | 3.3 V | 20 |
| VL53L0X direito | 9.7 | 17.5 | 3.3 V | 20 |
| WiFi (AP sem transmitir) | 63.7 | 80.4 | 5.0 V | - |
| WiFi transmitindo 10 Hz | 6.8 | 8.6 | 5.0 V | - |
| Motor A a 50% | 20.2 | 20.0 | 6.0 V | - |
| Motor A de 50% para 100% | 10.0 | 9.9 | 6.0 V | - |
| Motor B a 100% | 128.3 | 126.9 | 6.0 V | 350 |
| Motor A a 100% (E8 - E6) | 30.2 | 29.9 | 6.0 V | 350 |
| **Total (ultima etapa)** | 338.5 | - | - | - |

Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.

| Medida | Valor |
|---|---|
| Duracao | 0.7 min (447 amostras) |
| Tensao | 7.35 V -> 7.33 V |
| Corrente media | 231.9 mA |
| Corrente maxima | 1041.6 mA |
| Carga consumida | 2.9 mAh (0.2% de 1500 mAh) |
| Energia | 0.021 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 0 -> 165.7 mA · 255 -> 233.0 mA

Eventos:
- 20:19:06 Motores em PWM 255
- 20:19:31 Aviso de sobrecorrente em 100 mA (padrao 2000 mA)
- 20:19:32 Sobrecorrente: 253 mA
- 20:19:44 Aviso de sobrecorrente em 2000 mA (padrao 2000 mA)

## Ensaio (24/10/2026 20:19)


| Medida | Valor |
|---|---|
| Duracao | 0.0 min (25 amostras) |
| Tensao | 7.35 V -> 7.33 V |
| Corrente media | 210.7 mA |
| Corrente maxima | 1041.6 mA |
| Carga consumida | 0.2 mAh (0.0% de 1500 mAh) |
| Energia | 0.001 Wh |
| Reinicios do ESP | 0 |

Corrente media por PWM (sem o 1o segundo apos cada troca): 255 -> 227.8 mA

Eventos:
- 20:19:55 Motores parados
