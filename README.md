# README.md

# Sistema de Computador de Bordo - LASC 2026

## Visão Geral

Este projeto consiste no computador de bordo desenvolvido para a missão da **(LASC) 2026**.

O sistema é responsável por:

* Coletar dados de sensores embarcados.
* Receber imagens capturadas por um drone via rede Wi-Fi.
* Receber e transmitir telemetria através de rádio LoRa.
* Armazenar dados críticos em memória EEPROM.
* Encaminhar imagens e informações para a estação base.
* Gerenciar comunicação entre subsistemas da missão.
* Executar múltiplas tarefas simultaneamente utilizando FreeRTOS.

Durante a operação, um drone sobrevoará a área de interesse e enviará imagens para o computador de bordo através de uma rede Wi-Fi dedicada. Simultaneamente, os módulos embarcados coletarão informações de navegação e ambientais, que serão transmitidas para a estação terrestre por meio do enlace de telemetria.

---

# Arquitetura Geral

```text
                ┌─────────────┐
                │    Drone    │
                │ (Câmera)    │
                └──────┬──────┘
                       │ WiFi
                       ▼
             ┌──────────────────┐
             │ Computador de    │
             │ Bordo (ESP32)    │
             └──────┬───────────┘
                    │
      ┌─────────────┼─────────────┐
      │             │             │
      ▼             ▼             ▼
   Sensores       LoRa         EEPROM
      │             │             │
      └──────┬──────┴──────┬──────┘
             ▼             ▼
        Estação Base   Armazenamento
```

---

# Principais Funcionalidades

## Aquisição de Telemetria

O sistema coleta continuamente:

### GPS

* Latitude
* Longitude
* Número de satélites

### Sensores Ambientais

* Temperatura
* Umidade
* Pressão atmosférica
* Altitude estimada

### Unidade Inercial (IMU)

* Aceleração no eixo X
* Aceleração no eixo Y
* Aceleração no eixo Z
* Velocidade angular / Giroscópio (Roll, Pitch, Yaw)

### Medições de Energia e Bateria

* Temperatura da Bateria 1
* Temperatura da Bateria 2
* Tensão do barramento
* Corrente consumida

---

## Comunicação LoRa

Responsável pela transmissão de:

* Dados de telemetria
* Estados do sistema
* Respostas de subsistemas
* Possível transmissão de imagens fragmentadas

Características:

* Comunicação de longa distância
* Baixo consumo energético
* Operação independente da rede Wi-Fi

---

## Recepção de Imagens

O módulo Wi-Fi:

* Conecta-se ao drone.
* Solicita novas imagens.
* Recebe os frames.
* Armazena os dados em buffer.
* Encaminha para a estação base via interface serial.

---

## Armazenamento em EEPROM

O sistema salva informações importantes da missão para:

* Recuperação pós-voo.
* Backup da telemetria.
* Análise posterior.

O armazenamento é realizado apenas quando critérios operacionais são atendidos.

---

## Comunicação entre Subsistemas

Existem canais de comunicação destinados a:

### Controle de Atitude

Recebe e envia comandos relacionados à estabilização e navegação.

### Sistema de Energia

Recebe e envia informações sobre alimentação elétrica e gerenciamento energético.

---

## Multitarefa com FreeRTOS

O sistema utiliza tarefas independentes para garantir:

* Coleta contínua dos sensores.
* Comunicação simultânea.
* Processamento de imagens.
* Armazenamento em memória.

---

# Estrutura do Projeto

Todo o código-fonte do Computador de Bordo (OBC) está contido no diretório `mainV1.4/`.

## Arquivo Principal

### `mainV1.4/mainV1.4.ino`

Ponto de entrada do sistema.

Responsabilidades:

* Inicialização do ESP32.
* Inicialização do barramento I2C.
* Criação das filas de comunicação.
* Criação das tarefas FreeRTOS.
* Gerenciamento geral do sistema.

---

# Módulos de Sensores

## `gps.h`

Declarações do módulo GPS.

## `gps.cpp`

Responsável por:

* Inicializar o receptor GPS.
* Decodificar mensagens NMEA.
* Atualizar latitude, longitude e quantidade de satélites.

---

## `mpu.h`

Interface do módulo MPU6050.

## `mpu.cpp`

Responsável por:

* Inicializar o MPU6050.
* Ler aceleração dos eixos.
* Aplicar filtro de média móvel para suavização dos dados.

---

## `SensoresAmbientais.h`

Define a classe responsável pelos sensores ambientais.

## `SensoresAmbientais.cpp`

Responsável por:

### DHT22

* Temperatura
* Umidade

### BME280

* Pressão atmosférica
* Altitude estimada

---

# Sistema de Telemetria

## `telemetria.h`

Define:

* Estruturas de telemetria.
* Estruturas de imagens.
* Classe principal de comunicação LoRa.
* Tipos de pacotes.

---

## `telemetria.cpp`

Implementa:

* Configuração do rádio SX1262.
* Envio de pacotes LoRa.
* Recepção de pacotes.
* Controle de transações de imagem.
* Gerenciamento do estado do rádio.

---

# Gerenciamento de Rede

## `NetworkManager.h`

Interface do sistema de rede Wi-Fi.

## `NetworkManager.cpp`

Responsável por:

* Conexão com o ponto de acesso do drone.
* Monitoramento da conexão.
* Recepção de imagens.
* Gerenciamento de buffers.
* Controle da missão de captura.

---

# Ponte Serial

## `SerialBridge.h`

Declarações da ponte de transmissão serial.

## `SerialBridge.cpp`

Responsável por:

* Encaminhar imagens recebidas.
* Formatar cabeçalhos de transmissão.
* Transmitir buffers binários.
* Sinalizar início e fim dos frames.

Formato enviado:

```text
START:index:size:totalIndex:missionTime
[dados binários]
END_FRAME
```

---

# Armazenamento EEPROM

## `eepromI2C.h`

Define:

* Configurações da EEPROM.
* Funções de leitura e escrita.

## `eepromI2C.cpp`

Implementa:

* Escrita em páginas.
* Leitura em blocos.
* Controle de endereçamento.

---

# Comunicação com Subsistemas

## `slaves.h`

Interface de comunicação com módulos auxiliares.

## `slaves.cpp`

Responsável por:

* Enviar comandos.
* Receber respostas.
* Gerenciar portas seriais secundárias.

Subsistemas suportados:

* Controle de atitude
* Suprimento de energia

---

# Tarefas FreeRTOS

## `taskSensores.h`

Declarações da tarefa de sensores.

## `taskSensores.cpp`

Responsável por:

* Inicializar sensores.
* Coletar dados periodicamente.
* Enviar dados para fila de telemetria.

---

## `taskTelemetria.h`

Declarações da tarefa de telemetria.

## `taskTelemetria.cpp`

Responsável por:

* Inicializar LoRa.
* Processar filas.
* Enviar telemetria.
* Receber mensagens.
* Integrar informações dos subsistemas.

---

## `taskEeprom.h`

Declarações da tarefa de armazenamento.

## `taskEeprom.cpp`

Responsável por:

* Receber dados da fila.
* Avaliar condições de gravação.
* Persistir informações na EEPROM.

---

## `taskWifi.h`

Declarações da tarefa Wi-Fi.

## `taskWifi.cpp`

Responsável por:

* Inicializar rede.
* Solicitar imagens.
* Verificar recebimento.
* Sinalizar disponibilidade de novos frames.

---

# Protocolo Compartilhado

## `shared/protocol.h`

Contém as definições do protocolo de rede e funções auxiliares compartilhadas entre o Computador de Bordo e a Estação de Solo. Centraliza os seguintes elementos para garantir sincronismo nos dados transmitidos via LoRa:

*   **Tipos de Pacote:** `TYPE_SENSOR` (0x01), `TYPE_GPS` (0x02), `TYPE_GYRO` (0x03), `TYPE_IMAGE` (0x10), `TYPE_DEBUG` (0x20), `TYPE_COMMAND` (0x30).
*   **Endereços de Origem/Destino:** `ADDR_GROUND` (0x01) e `ADDR_OBC` (0x02).
*   **Estrutura unificada de Sensores:** `struct sensorsData` empacotada com alinhamento de 1 byte (`#pragma pack(push, 1)`).
*   **Helpers Utilitários:** Funções inline para construir cabeçalho (`buildHeader`), validar tamanho/início (`validateHeader`), ler campos (`packetType`, `packetSrc`, `packetDst`), extrair payload (`packetPayload`, `packetPayloadSize`), e realizar parse seguro (`parseSensorData`, `parseDebugMessage`).

---

# Estação de Solo (Ground Station)

## `LoRaRX/LoRaRX.ino`

Módulo responsável pela recepção de dados transmitidos pelo computador de bordo e monitoramento da missão em tempo real a partir do solo.

Funcionalidades e Responsabilidades:
*   **Configuração do Rádio:** Opera em placa Heltec V2 utilizando chip SX1276 configurado na frequência de 915.0 MHz.
*   **Validação de Pacotes:** Utiliza as funções de validação de `shared/protocol.h` para inspecionar a integridade de cabeçalho e início (`START_BYTE`).
*   **Processamento Multitipos:**
    *   **Sensores (`TYPE_SENSOR`):** Desempacota e imprime temperatura, umidade, altitude, pressão, coordenadas GPS, satélites, dados de giroscópio (Roll/Pitch/Yaw) e medições de bateria/energia (temperatura das baterias, tensão e corrente).
    *   **Imagens (`TYPE_IMAGE`):** Rastreia e reporta o recebimento de chunks de imagem fragmentados.
    *   **Comandos (`TYPE_COMMAND`):** Registra comandos enviados para o OBC.
    *   **Logs de Depuração (`TYPE_DEBUG`):** Recebe e exibe no monitor serial mensagens de depuração enviadas em tempo real pelo OBC.
*   **Estatísticas e Diagnósticos:** Computa e exibe contadores detalhados de pacotes (completos, incompletos, desconhecidos), RSSI e SNR de recepção.

---

# Fluxo Operacional

```text
Sensores
    │
    ▼
Task Sensores
    │
    ▼
Fila de Telemetria
    │
    ├────────► Task EEPROM
    │              │
    │              ▼
    │         EEPROM
    │
    ▼
Task Telemetria
    │
    ▼
LoRa
    │
    ▼
Estação Base


Drone
    │
    ▼
WiFi
    │
    ▼
Task WiFi
    │
    ▼
Buffer de Imagem
    │
    ▼
SerialBridge
    │
    ▼
Estação Base
```

---

# Hardware Utilizado

* ESP32
* SX1262 LoRa
* GPS
* MPU6050
* DHT22
* BME280
* EEPROM I2C
* Drone com câmera Wi-Fi


---

# Processo de Debug (Depuração)

O sistema possui um mecanismo unificado e estruturado de depuração através do módulo `DebugLog` (`DebugLog.h` / `DebugLog.cpp`). Este módulo fornece logs dinâmicos e controle de severidade para facilitar a identificação de falhas em múltiplos subsistemas rodando sob o FreeRTOS.

## Níveis de Severidade

Os logs são categorizados em três níveis de severidade (definidos no enum `DebugLevel`):
*   `DBG_INFO` ('I'): Mensagens informacionais sobre o fluxo padrão do sistema.
*   `DBG_WARN` ('W'): Avisos de comportamento inesperado ou operação degradada (ex: sensor indisponível), mas com o sistema ainda operacional.
*   `DBG_ERROR` ('E'): Erros críticos onde um módulo ou funcionalidade falhou por completo.

## Macros de Conveniência por Módulo

Para simplificar a escrita de código e padronizar os prefixos dos logs, cada módulo possui sua própria macro dedicada:
*   `DBG_GPS(...)` (Módulo GPS)
*   `DBG_MPU(...)` (Módulo MPU6050)
*   `DBG_DHT(...)` (Sensor DHT22)
*   `DBG_BME(...)` (Sensor BME280)
*   `DBG_EEPROM(...)` (Memória EEPROM)
*   `DBG_LORA(...)` (Comunicação LoRa)
*   `DBG_WIFI(...)` (Comunicação Wi-Fi)
*   `DBG_BRIDGE(...)` (Ponte Serial)
*   `DBG_SLAVES(...)` (Subsistemas Auxiliares)

As mensagens são formatadas no padrão `[MÓDULO][SEVERIDADE] mensagem` (com tamanho máximo de 200 caracteres para garantir compatibilidade com pacotes LoRa).

## Roteamento de Saída Dupla (Dual Output)

Toda chamada à função `debugLog()` envia a informação para duas saídas distintas simultaneamente:

1.  **Conexão Serial (Imediata):** O log é impresso na porta serial via `Serial.println()` instantaneamente. Essa operação é não-bloqueante.
2.  **Transmissão LoRa (Fila FreeRTOS):** A mensagem é empacotada na estrutura `DebugMsg` e enviada à fila global `xDebugQueue`. O envio para a fila utiliza tempo limite zero (não bloqueante). Se a fila estiver cheia (limite de 16 mensagens), a mensagem mais recente é descartada silenciosamente para evitar que tarefas críticas de telemetria ou leitura de sensores travem.

## Inicialização e Processamento em Segundo Plano

*   **Inicialização:** O sistema de debug deve ser inicializado chamando `debugInit()` antes de qualquer chamada de log. Atualmente, isso é feito na inicialização da tarefa de sensores (`taskSensores.cpp`).
*   **Envio via Rádio:** A tarefa de telemetria (`taskTelemetria.cpp`) monitora continuamente a fila `xDebugQueue`. Quando o rádio LoRa está ocioso (`lora.isIdle()`), a tarefa retira a mensagem de debug da fila e a transmite via rádio com o identificador de tipo `TYPE_DEBUG`, permitindo a monitoração em tempo real a partir da estação base.

---

# Observações para Desenvolvedores

Antes de modificar o código:

1. Verifique as filas FreeRTOS utilizadas entre tarefas.
2. Analise impactos na temporização das tarefas.
3. Mantenha compatibilidade com os formatos de telemetria já definidos.
4. Evite alterações nos protocolos de comunicação sem atualizar a estação base.
5. Certifique-se de que novas funcionalidades não bloqueiem as tarefas críticas.

Este documento foi elaborado para facilitar a manutenção e evolução do sistema por desenvolvedores externos.