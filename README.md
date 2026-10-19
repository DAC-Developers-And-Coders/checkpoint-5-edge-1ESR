# 🍷 O Caso da Vinheria Agnello — Sistema de Monitoramento Ambiental IoT

Projeto de Edge Computing para monitoramento das condições ambientais de uma adega, desenvolvido com ESP32, sensores de temperatura, umidade e luminosidade, comunicação MQTT e integração com a plataforma FIWARE.

O sistema coleta dados do ambiente, classifica as condições por níveis de alerta, sinaliza o estado por meio de LEDs e envia as leituras para um servidor remoto, onde os dados são disponibilizados por meio de uma API NGSIv2 e armazenados para consulta histórica.

---

## 📌 Descrição do Projeto

O objetivo é monitorar as condições ambientais da Vinheria Agnello e permitir que as informações coletadas pelos sensores sejam consultadas remotamente.

O ESP32 realiza leituras periódicas dos sensores e utiliza uma conexão Wi-Fi para publicar os dados por MQTT.

Além do acesso aos valores atuais, o projeto utiliza o STH-Comet para armazenar e consultar o histórico das leituras.

O sistema também conta com três LEDs para indicar as condições ambientais:

- 🟢 **Verde:** condições adequadas.
- 🟡 **Amarelo:** condições de atenção.
- 🔴 **Vermelho:** condições críticas.

---

## 🛠️ Tecnologias e Componentes

### Hardware e simulação

- ESP32;
- Sensor DHT22 para temperatura e umidade;
- Sensor LDR para luminosidade;
- LED verde;
- LED amarelo;
- LED vermelho;
- Simulador Wokwi.

### Comunicação e backend

- Wi-Fi;
- MQTT;
- FIWARE IoT Agent for Ultralight;
- Orion Context Broker;
- STH-Comet para armazenamento histórico;
- MongoDB;
- Docker;
- API REST com NGSIv2;
- VM hospedada na Azure.

### Linguagens e bibliotecas

- C/C++;
- `WiFi.h`;
- `PubSubClient.h`;
- `DHT.h`;
- `ctime`.

#### libraries.txt (Wokwi)
```
# Wokwi Library List
# See https://docs.wokwi.com/guides/libraries

# Automatically added based on includes:
DHT sensor library

WiFi
PubSubClient
```
---

## ⚙️ Arquitetura do Sistema

O fluxo de dados do projeto é organizado da seguinte maneira:

1. **Coleta:** o ESP32 lê a temperatura, a umidade relativa do ar e a luminosidade.
2. **Processamento local:** os valores são avaliados para determinar o nível de alerta indicado pelos LEDs.
3. **Data e hora:** o dispositivo obtém o horário por meio de servidores NTP e associa um timestamp às leituras.
4. **Publicação MQTT:** os dados são enviados ao broker MQTT por Wi-Fi.
5. **Conversão dos dados:** o FIWARE IoT Agent recebe as mensagens no formato UltraLight e as converte para entidades NGSIv2.
6. **Disponibilização:** o Orion Context Broker mantém os valores atuais dos sensores para consulta via API REST.
7. **Armazenamento histórico:** uma assinatura do Orion encaminha as atualizações ao STH-Comet, permitindo consultar leituras anteriores.

---

## 🌡️ Sensores e Dados Coletados

| Atributo | Sensor | Unidade ou representação |
|---|---|---|
| Temperatura | DHT22 | °C |
| Umidade | DHT22 | % de umidade relativa |
| Luminosidade | LDR | Escala percentual de 0 a 100 |

O sensor LDR utiliza a leitura analógica do ESP32, com valores convertidos para uma escala de 0 a 100. Essa escala representa uma estimativa relativa de luminosidade, não uma medição calibrada em lux.

As leituras são realizadas a cada **2 segundos**, conforme o intervalo configurado no firmware.

---

## 🚦 Sistema de Alertas

O firmware avalia os valores dos sensores e define o estado dos LEDs com base em limites configurados.

### Limites utilizados

| Parâmetro | Limite configurado |
|---|---|
| Luminosidade | 30% |
| Temperatura máxima de referência | 15 °C |
| Temperatura mínima de referência | 12 °C |
| Umidade mínima de referência | 60% |
| Umidade máxima de referência | 80% |

Os valores são utilizados em condições que determinam os estados de alerta, incluindo faixas de atenção próximas aos limites.

- 🟢 **Verde:** condições dentro das faixas consideradas adequadas pelo código.
- 🟡 **Amarelo:** valores próximos aos limites definidos.
- 🔴 **Vermelho:** valores que ultrapassam os limites críticos configurados.

Os limites são parâmetros do firmware e podem ser ajustados conforme os requisitos de conservação dos produtos armazenados.

---

## 📡 Comunicação MQTT

A comunicação entre o ESP32 e o backend utiliza o protocolo MQTT.

O dispositivo publica suas leituras no tópico:

```text
/TEF/device014/attrs
```

Também realiza a inscrição no tópico de comandos:

```text
/TEF/device014/cmd
```

O identificador do dispositivo é `device014`, e a API Key utilizada pelo serviço é `TEF`.

### Formato das mensagens

As mensagens são enviadas no formato UltraLight, com timestamp, identificadores dos atributos e respectivos valores.

Exemplo ilustrativo:

```text
2026-10-18T20:10:00-03:00|t|14.0|h|68.0|l|9
```

Nesse exemplo:

- `2026-10-18T20:10:00-03:00`: data e hora da leitura.
- `t|14.0`: temperatura de 14 °C.
- `h|68.0`: umidade relativa de 68%.
- `l|9`: luminosidade relativa de 9%.

O horário do exemplo é ilustrativo. O dispositivo utiliza sincronização NTP para obter a data e a hora atuais.

---

## ☁️ Integração com o FIWARE

O backend utiliza componentes FIWARE para receber, disponibilizar e armazenar os dados dos sensores.

### FIWARE IoT Agent

Recebe as mensagens MQTT do dispositivo e converte os atributos do formato UltraLight para entidades compatíveis com NGSIv2.

### Orion Context Broker

Armazena o estado atual da entidade e permite consultar os atributos por meio de requisições HTTP.

A entidade registrada para o dispositivo é:

```text
ID: urn:ngsi-ld:Thing:device014
Tipo: Thing
```

Os atributos disponibilizados são:

- `temperature`
- `humidity`
- `luminosity`
- `TimeInstant`, quando disponibilizado nos metadados da entidade.

### Consulta dos dados atuais

Com os serviços FIWARE em execução no servidor, os dados podem ser consultados por meio da API NGSIv2:

```bash
curl -s \
  -H 'fiware-service: openiot' \
  -H 'fiware-servicepath: /' \
  'http://localhost:1026/v2/entities?options=keyValues'
```

Para consultar somente a entidade do dispositivo:

```bash
curl -s \
  -H 'fiware-service: openiot' \
  -H 'fiware-servicepath: /' \
  'http://localhost:1026/v2/entities/urn:ngsi-ld:Thing:device014?options=keyValues'
```

Essas requisições retornam os valores atuais disponibilizados pelo Orion Context Broker.

---

## 🕒 Timestamp das Leituras

Cada mensagem publicada pelo ESP32 inclui uma data e hora obtidas por sincronização NTP.

O firmware utiliza o fuso horário UTC−03:00 para formatar o timestamp. Caso não consiga obter a hora, a leitura não é publicada.

A inclusão do timestamp permite identificar o momento associado à leitura. Para confirmar que o horário de origem é preservado durante todo o processamento, é necessário comparar o timestamp publicado pelo ESP32 com os metadados retornados pelo Orion.

O STH-Comet também registra o horário de recebimento das atualizações, disponibilizado no campo `recvTime`. Esse horário representa o recebimento pelo serviço histórico e não deve ser confundido automaticamente com o timestamp original do sensor.

---

## 📈 Armazenamento e Consulta Histórica

O STH-Comet é utilizado para armazenar o histórico dos atributos monitorados. Uma assinatura configurada no Orion encaminha as atualizações de temperatura, umidade e luminosidade para o serviço histórico.

O histórico permite consultar as últimas leituras de cada atributo.

### Exemplo: consultar as últimas cinco temperaturas

```bash
curl -s \
  -H 'fiware-service: openiot' \
  -H 'fiware-servicepath: /' \
  'http://localhost:8666/STH/v1/contextEntities/type/Thing/id/urn:ngsi-ld:Thing:device014/attributes/temperature?lastN=5'
```

Para consultar a umidade ou a luminosidade, basta substituir `temperature` pelo atributo desejado na URL.

O histórico permite acompanhar variações ao longo do tempo e verificar se os dados estão sendo recebidos e armazenados corretamente.

---

## 🧪 Como Executar e Testar

### 1. Executar o circuito

Abra o projeto no Wokwi e inicie a simulação do ESP32 com os sensores conectados aos pinos definidos no firmware.

### 2. Verificar a conexão Wi-Fi

O dispositivo utiliza a rede de simulação `Wokwi-GUEST`. No Monitor Serial, verifique a mensagem de conexão bem-sucedida.

### 3. Verificar a comunicação MQTT

Confirme no Monitor Serial se aparece a mensagem de conexão ao MQTT e se as leituras são publicadas no tópico `/TEF/device014/attrs`.

### 4. Consultar os dados no Orion

Execute uma das requisições `curl` da seção de integração FIWARE para verificar os valores atuais dos sensores.

### 5. Verificar o histórico

Consulte o endpoint do STH-Comet para conferir se as últimas leituras foram armazenadas.

### 6. Verificar os LEDs

Altere as condições dos sensores no simulador e observe se os LEDs refletem os níveis de alerta definidos no firmware.

---

## ✅ Funcionalidades Implementadas

- Monitoramento de temperatura, umidade e luminosidade.
- Leitura periódica dos sensores pelo ESP32.
- Indicação visual das condições ambientais por meio de LEDs.
- Conexão Wi-Fi e reconexão quando necessário.
- Publicação de dados por MQTT.
- Inclusão de timestamp nas mensagens publicadas.
- Recepção e conversão de dados pelo FIWARE IoT Agent.
- Disponibilização dos dados atuais via API NGSIv2.
- Armazenamento histórico por meio do STH-Comet.
- Consulta das últimas leituras dos sensores por HTTP.

---

## 🔗 Simulação Wokwi

[https://wokwi.com/projects/477722266498186241](https://wokwi.com/projects/477722266498186241)

---

## 👥 Equipe

- **ANDRÉ VICTOR GONÇALVES NASCIMENTO** - RM 570567
- **DAVI DIAS DE SOUZA FREITAS** - RM 574089
- **DAVID MIKAEL DIAS DA SILVA** - RM 571637
- **GABRIEL NOVAGA PEREIRA** - RM 573196
- **MATHEUS MONTEIRO DA SILVA** - RM 573842
