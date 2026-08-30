#include <Arduino.h>

#define ENDERECO_CONTROLE 3

// Pinos do driver MX1616 (ponte H) ligados ao motor DC.
// HIGH/LOW  -> gira num sentido (abrir)
// LOW/HIGH  -> gira no sentido oposto (fechar)
// LOW/LOW   -> motor parado (freio/coast)
#define PINO_M1_IN1 8 
#define PINO_M1_IN2 9 

#define PINO_M2_IN1 10
#define PINO_M2_IN2 11

#define STATUS_PARADO 0 //MOTORES DESLIGADOS
#define STATUS_M1_HOR 1 //MOTOR 1 GIRANDO NO SENTIDO HORÁRIO
#define STATUS_M1_ANT 2 // MOTOR 1 GIRANDO NO SENTIDO ANTI-HORARIO
#define STATUS_M2_HOR 3 //MOTOR 2 GIRANDO NO SENTIDO HORARIO
#define STATUS_M2_ANT 4 //MOTOR 2 GIRANDO ANTI-HORARIO


int statusAtual = STATUS_PARADO;

// --- Auto-stop nao blocoante -------------------------------------
const unsigned long TEMPO_MOVIMENTO_MS = 2000; // ajuste ao tempo real do curso
bool movimentoAtivo = false;
unsigned long inicioMovimento = 0;
// --------------------------------------------------------------------

void pararMotores()
{
    digitalWrite(PINO_M1_IN1, LOW);
    digitalWrite(PINO_M1_IN2, LOW);
    digitalWrite(PINO_M2_IN1, LOW); // CORRIGIDO: Era PINO_M2_IN2 repetido
    digitalWrite(PINO_M2_IN2, LOW);
    movimentoAtivo = false;
    statusAtual = STATUS_PARADO; 
}

void motor1Horario()
{
    pararMotores();
    digitalWrite(PINO_M1_IN1, HIGH);
    digitalWrite(PINO_M1_IN2, LOW);
    movimentoAtivo = true;
    inicioMovimento = millis();
    statusAtual = STATUS_M1_HOR; 
}

void motor1AntiHorario() 
{ 
    pararMotores(); 
    digitalWrite(PINO_M1_IN1, LOW);
    digitalWrite(PINO_M1_IN2, HIGH);
    movimentoAtivo = true;
    inicioMovimento = millis(); 
    statusAtual = STATUS_M1_ANT;
}

void motor2Horario() 
{ 
    pararMotores(); 
    digitalWrite(PINO_M2_IN1, HIGH);
    digitalWrite(PINO_M2_IN2, LOW);
    movimentoAtivo = true;
    inicioMovimento = millis(); 
    statusAtual = STATUS_M2_HOR;
}

void motor2AntiHorario() 
{ 
    pararMotores(); 
    digitalWrite(PINO_M2_IN1, LOW);
    digitalWrite(PINO_M2_IN2, HIGH);
    movimentoAtivo = true;
    inicioMovimento = millis(); 
    statusAtual = STATUS_M2_ANT;
}

void enviarStatus()
{
    Serial.print(ENDERECO_CONTROLE);
    Serial.print(':');
    Serial.println(statusAtual);
}

void setup()
{
    Serial.begin(115200); 
    pinMode(PINO_M1_IN1, OUTPUT);
    pinMode(PINO_M1_IN2, OUTPUT);

    pinMode(PINO_M2_IN1, OUTPUT);
    pinMode(PINO_M2_IN2, OUTPUT);

    pararMotores(); 
}

void loop()
{
    if (movimentoAtivo && (millis() - inicioMovimento >= TEMPO_MOVIMENTO_MS))
    {
        pararMotores(); 
        enviarStatus(); 
    }

    if (Serial.available())
    {
        String linha = Serial.readStringUntil('\n');
        linha.trim();

        int comando = linha.toInt();

        switch (comando)
        {
            case 0: 
                pararMotores();
                enviarStatus();
                break;
            case 1: 
                motor1Horario();
                enviarStatus(); 
                break;

            case 2: 
                motor1AntiHorario(); 
                enviarStatus(); 
                break;

            case 3: 
                motor2Horario(); // CORRIGIDO: Letra inicial alterada para "m" minúsculo
                enviarStatus();
                break;

            case 8: 
                motor2AntiHorario(); 
                enviarStatus(); 
                break;
            
            case 9: 
                enviarStatus(); 
                break;
            
            default: 
                break;
        }
    }
}