# ESP32-S3 Dual INMP441 Audio Forwarder (Edge Impulse)

Este projeto implementa a coleta de áudio estéreo com downmix para mono, utilizando dois microfones **INMP441** e um **ESP32-S3** via **ESP-IDF v5.5.3**. O firmware é otimizado para o envio de dados via Serial para o **Edge Impulse Data Forwarder**.

## Características Técnicas
* **Hardware:** 2x Microfones I2S INMP441.
* **Frequência de Amostragem:** 16.000 Hz.
* **Processamento:** Leitura em Estéreo -> Conversão PCM 16-bit -> Downmix para Mono.
* **Arquitetura:** Task dedicada no Core 1 com prioridade de tempo real.

---

## Esquema de Ligação (Wiring)

Ambos os microfones compartilham o barramento, diferenciando-se pelo pino **L/R**.

| Componente | Pino Microfone | ESP32-S3 GPIO |
| :--- | :--- | :--- |
| **I2S SCK** | SCK / CLK | **GPIO 16** |
| **I2S WS** | WS / Word Select | **GPIO 15** |
| **I2S SD** | SD / Data Out | **GPIO 17** |
| **Mic 1 (Left)** | **L/R** | **GND** |
| **Mic 2 (Right)** | **L/R** | **3.3V** |
| **Alimentação** | VDD / GND | 3.3V / GND |

---

## Pontos de Atenção

### 1. Baud Rate do Monitor Serial
Por padrão, o ESP-IDF usa 115200. No entanto, enviar 16.000 números por segundo via `printf` pode sobrecarregar a largura de banda da serial e causar atrasos.

**Recomendação:** Se o Data Forwarder reclamar de perda de dados, aumente o baud rate para **921600** no seu `sdkconfig` e no comando da CLI:

```bash
edge-impulse-data-forwarder --baudrate 921600
```

### 2. Nível de Log do ESP-IDF
Se o ESP-IDF começar a imprimir mensagens de sistema (como avisos de Wi-Fi ou Memória) no meio dos dados, o Edge Impulse vai ler isso como erro.

**Ação:** No `app_main`, antes de iniciar a captura, você pode desativar logs globais para limpar a saída serial:

```c
esp_log_level_set("*", ESP_LOG_NONE);
```

### 3. Os Pinos GPIO
Os pinos configurados neste projeto são:
* **WS:** 15
* **SD:** 17
* **SCK:** 16

Certifique-se de que esses pinos na sua placa ESP32-S3 não conflitem com memórias flash internas ou pinos de boot (embora no S3 esses GPIOs costumam ser seguros).

---

## Como validar se o dado está bom no Edge Impulse

Após conectar o dispositivo ao seu projeto no Edge Impulse Studio, realize os seguintes testes na aba **Data Acquisition**:

1.  **Verifique a forma de onda:** O gráfico deve mostrar uma linha reta próxima ao zero durante o silêncio absoluto.
2.  **Teste de Saturação:** Dê um **Clap** (palma) bem perto do microfone. Se o topo da onda no gráfico ficar "achatado" (cortado em linha reta no topo), aumente o shift de `>> 14` para `>> 15`. Se a onda ficar imperceptível, diminua para `>> 12`.
3.  **Teste de Frequência:** Faça um **Whistle** (assobio). Você deve observar uma onda senoidal bem definida, limpa e rítmica no gráfico, sem ruídos aleatórios (serrilhados).

---

## Como rodar

1. Compile e grave o firmware:
   ```bash
   idf.py build flash
   ```
2. Feche qualquer monitor serial aberto.
3. Inicie o forwarder:
   ```bash
   edge-impulse-data-forwarder --baudrate 921600
   ```
4. Configure a frequência como **16000Hz** no prompt do CLI.
