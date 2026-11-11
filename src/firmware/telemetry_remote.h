#pragma once
#include <Arduino.h>
void iniciarTelemetriaRemota();
void atualizarSnapshotRemoto(); // chamar apos controlador; nao realiza rede
String remotoDiagnosticoJson();
