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

## Arquivo Principal

### `mainV1.3.ino`

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

# Observações para Desenvolvedores

Antes de modificar o código:

1. Verifique as filas FreeRTOS utilizadas entre tarefas.
2. Analise impactos na temporização das tarefas.
3. Mantenha compatibilidade com os formatos de telemetria já definidos.
4. Evite alterações nos protocolos de comunicação sem atualizar a estação base.
5. Certifique-se de que novas funcionalidades não bloqueiem as tarefas críticas.

Este documento foi elaborado para facilitar a manutenção e evolução do sistema por desenvolvedores externos.