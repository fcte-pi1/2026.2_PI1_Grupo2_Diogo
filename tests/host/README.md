# Smoke tests host (sem hardware)

Estes testes compilam dois modulos C++ com `Arduino.h` mockado e simulam respostas ideias para os motores. **Nao representam o comportamento fisico do ESP32**.

Na raiz do projeto, em Linux com g++:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Itests/host -Isrc/firmware src/firmware/maze_state.cpp tests/host/test_maze.cpp -o /tmp/test_maze && /tmp/test_maze
g++ -std=c++17 -Wall -Wextra -Werror -Itests/host -Isrc/firmware src/firmware/motion_control.cpp tests/host/test_motion.cpp -o /tmp/test_motion && /tmp/test_motion
g++ -std=c++17 -Wall -Wextra -Werror -Itests/host -Isrc/firmware src/firmware/motion_control.cpp tests/host/test_giro.cpp -o /tmp/test_giro && /tmp/test_giro
g++ -std=c++17 -Wall -Wextra -Werror -Itests/host -Isrc/firmware src/firmware/motion_control.cpp tests/host/test_partida.cpp -o /tmp/test_partida && /tmp/test_partida
```
