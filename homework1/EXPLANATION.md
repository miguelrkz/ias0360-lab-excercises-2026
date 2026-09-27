# Guía Completa de Comprensión: Homework 1 (IMU Feature Extraction)

Esta guía explica en detalle y de forma sencilla **qué hace cada archivo**, **cómo funciona el código**, **las matemáticas involucradas**, **cómo se ejecuta todo** y **cómo defender el proyecto en una presentación o examen**.

---

## 1. Visión General del Proyecto

### ¿Cuál es el problema que resolvemos?
Un sensor inercial (IMU) como el **ICM-20948** mide aceleración ($a_x, a_y, a_z$) y velocidad angular ($g_x, g_y, g_z$) a alta velocidad (500 veces por segundo, 500 Hz).
Si enviamos o guardamos cada dato individual continuamente:
- Saturamos la tarjeta SD y los buses de comunicación.
- Gastamos demasiada batería y memoria.
- Los datos crudos tienen mucho ruido de alta frecuencia.

### ¿Cuál es la solución?
**Extracción de características (Feature Extraction) en el propio microcontrolador (RP2350)**:
En lugar de procesar muestra a muestra, agrupamos las lecturas en **ventanas de 256 muestras** ($\approx 512\text{ ms}$, medio segundo). Para cada ventana, calculamos un conjunto reducido de números resumen (media, desviación estándar, picos de frecuencia, energía, etc.). 
Esto reduce el volumen de datos de miles de bytes a solo unos pocos números clave que luego pueden alimentar un clasificador de Machine Learning o guardarse eficientemente.

---

## 2. Configuración del Sensor y Conversión de Unidades

### A. ¿Qué significan LSB, LSB/g y LSB/(°/s)?
- **LSB (Least Significant Bit / Bit Menos Significativo):** Es la unidad mínima discreta que el Convertidor Analógico-Digital (ADC) interno de 16 bits del sensor puede medir. Como el ADC tiene 16 bits con signo, su rango de salida va desde $-32768$ hasta $+32767$ cuentas (LSB).
- **Sensibilidad (LSB/g):** Indica cuántas cuentas digitales (LSB) genera el sensor por cada $1g$ de aceleración física ($1g \approx 9.81\text{ m/s}^2$).

### B. Configuración de Acelerómetro a $\pm 2g$ (¿Por qué dividimos entre 16384?)
En el archivo [`icm20948/icm20948.c:247-248`](icm20948/icm20948.c):
```c
I2C_WriteOneByte(REG_ADD_ACCEL_CONFIG,
                 REG_VAL_BIT_ACCEL_DLPCFG_6 | REG_VAL_BIT_ACCEL_FS_2g | REG_VAL_BIT_ACCEL_DLPF);
```
- Se escribe `REG_VAL_BIT_ACCEL_FS_2g` en el registro `ACCEL_CONFIG` (Bank 2).
- Esto configura el acelerómetro en su rango de escala completa de **$\pm 2g$** (desde $-2g$ hasta $+2g$).
- Como el entero de 16 bits con signo abarca $32768$ niveles positivos y $32768$ negativos:
  $$\text{Sensibilidad} = \frac{32768\text{ LSB}}{2g} = 16384\text{ LSB}/g$$
- Por tanto, para obtener la aceleración real en unidades de $g$:
  $$a_x = \frac{\text{raw\_ax}}{16384} \quad [g]$$

### C. Configuración de Giroscopio a $\pm 1000^\circ/\text{s}$ (¿Por qué dividimos entre 32.8?)
En el archivo [`icm20948/icm20948.c:244-245`](icm20948/icm20948.c):
```c
I2C_WriteOneByte(REG_ADD_GYRO_CONFIG_1,   
                 REG_VAL_BIT_GYRO_DLPCFG_6 | REG_VAL_BIT_GYRO_FS_1000DPS | REG_VAL_BIT_GYRO_DLPF);
```
- Se escribe `REG_VAL_BIT_GYRO_FS_1000DPS` en el registro `GYRO_CONFIG_1`.
- Esto configura el giroscopio para medir velocidades angulares de hasta **$\pm 1000^\circ/\text{s}$** (grados por segundo).
- Calculando la sensibilidad:
  $$\text{Sensibilidad} = \frac{32768\text{ LSB}}{1000^\circ/\text{s}} = 32.768\text{ LSB}/(^\circ/\text{s}) \approx 32.8\text{ LSB}/(^\circ/\text{s})$$
- Por tanto, para obtener la velocidad angular real en grados por segundo ($^\circ/\text{s}$):
  $$\omega_x = \frac{\text{raw\_gx}}{32.8} \quad [^\circ/\text{s}]$$

---

## 3. ¿Sobre qué datos se aplican los algoritmos y qué características se generan?

### A. ¿Acelerómetro o Giroscopio?
1. **Adquisición cruda:** Se leen ambos sensores (los 3 ejes del acelerómetro $a_x, a_y, a_z$ y los 3 ejes del giroscopio $\omega_x, \omega_y, \omega_z$). Todas las muestras crudas se guardan en el archivo binario (`imu_data.bin`).
2. **Procesamiento Digital de Señales (DSP):** Las estadísticas temporales, la FFT en frecuencia y la cuantización Q15 se aplican **específicamente sobre la magnitud euclídea del vector de aceleración**:
   $$\|a\| = \sqrt{a_x^2 + a_y^2 + a_z^2}$$
   además de calcular la media de cada eje individual ($\mu_{a_x}, \mu_{a_y}, \mu_{a_z}$).
3. **¿Por qué sobre la magnitud $\|a\|$?**  
   Porque es **invariante ante rotaciones**: en reposo o inclinación estática, la gravedad siempre suma exactamente $1.0g$. Si analizáramos un solo eje (por ejemplo $a_z$), al girar la placa los algoritmos creerían falsamente que ha habido un movimiento dinámico.

### B. Las 23 Características Generadas en el CSV (`imu_features.csv`)
Por cada ventana de 256 muestras ($\approx 512\text{ ms}$), Core 0 escribe una fila con las siguientes **23 columnas**:

| Nº | Columna en CSV | Categoría | Significado Físico y Matemático |
| :---: | :--- | :--- | :--- |
| **1** | `window_id` | Identificación | Número secuencial de la ventana procesada ($0, 1, 2, \dots$). |
| **2** | `t_sec` | Identificación | Marca de tiempo transcurrida en segundos desde el encendido. |
| **3** | `ax_mean` | Orientación / Tilt | Media de aceleración en eje X. Refleja inclinación estática. |
| **4** | `ay_mean` | Orientación / Tilt | Media de aceleración en eje Y. Refleja inclinación estática. |
| **5** | `az_mean` | Orientación / Tilt | Media de aceleración en eje Z. Refleja inclinación estática. |
| **6** | `amag_mean` | Estadística ($\|a\|$) | Media de la magnitud. En reposo es $\approx 1.0g$. |
| **7** | `amag_std` | Estadística ($\|a\|$) | Desviación estándar $\sigma$. Mide la **intensidad del movimiento/vibración**. |
| **8** | `amag_min` | Estadística ($\|a\|$) | Valor mínimo de $\|a\|$ dentro de la ventana de 512 ms. |
| **9** | `amag_max` | Estadística ($\|a\|$) | Valor máximo de $\|a\|$ dentro de la ventana de 512 ms. |
| **10** | `amag_range` | Estadística ($\|a\|$) | Rango dinámico ($R = \max - \min$). Detecta picos de impacto. |
| **11** | `amag_median` | Estadística ($\|a\|$) | Mediana (calculada con Insertion Sort). Robusta ante picos de ruido. |
| **12** | `amag_rms` | Estadística ($\|a\|$) | Valor cuadrático medio. Potencia total efectiva de la señal. |
| **13** | `f_dom_hz` | Frecuencia (FFT) | **Frecuencia dominante** del movimiento (ej: $1.95\text{ Hz}$ en tapping). |
| **14** | `spectral_energy` | Frecuencia (FFT) | Energía armónica total $\sum \|X[k]\|^2$. |
| **15** | `p1_hz` | Frecuencia (FFT) | Frecuencia del 1.ᵉʳ pico con mayor amplitud espectral. |
| **16** | `p1_amp` | Frecuencia (FFT) | Amplitud del 1.ᵉʳ pico espectral en $g$. |
| **17** | `p2_hz` | Frecuencia (FFT) | Frecuencia del 2.º pico espectral. |
| **18** | `p2_amp` | Frecuencia (FFT) | Amplitud del 2.º pico espectral en $g$. |
| **19** | `p3_hz` | Frecuencia (FFT) | Frecuencia del 3.ᵉʳ pico espectral. |
| **20** | `p3_amp` | Frecuencia (FFT) | Amplitud del 3.ᵉʳ pico espectral en $g$. |
| **21** | `q15_snr_db` | Cuantización Q15 | Relación Señal a Ruido ($\text{SNR}$) en decibelios. |
| **22** | `q15_rms_err` | Cuantización Q15 | Error cuadrático medio de reconstrucción de Q15 frente al float. |
| **23** | `q15_clips` | Cuantización Q15 | Conteo de muestras que se salieron del rango $[-1.0, 1.0)$ y saturaron. |

---

## 4. Cuantización Q15, Normalización y Por Qué Shaking Desploma el SNR

### ¿Qué es el formato Q15?
Representa números decimales fraccionarios usando enteros con signo de 16 bits (`int16_t`, de $-32768$ a $+32767$).
Por convenio, el rango decimal representable es exactamente **$[-1.0, +1.0)$**:
- $+1.0 \rightarrow +32767$
- $-1.0 \rightarrow -32768$

### Fórmula de normalización utilizada
En [`main.c:174`](main.c), normalizamos la magnitud $\|a\|$ con:
$$x_{\text{norm}} = \frac{\|a\|}{2.0} - 0.5$$

- En **reposo o rotación lenta** ($\|a\| \approx 1.0g$):  
  $$x_{\text{norm}} = \frac{1.0}{2.0} - 0.5 = 0.0 \quad \text{(en el centro exacto)}$$
- En **tapping rítmico** ($\|a\| \approx 1.5g$):  
  $$x_{\text{norm}} = \frac{1.5}{2.0} - 0.5 = +0.25 \quad \text{(dentro de } [-1.0, 1.0)\text{)}$$

En ambos casos, la señal queda dentro del rango. El único error de conversión es un pequeñísimo redondeo de entero ($\approx 0.00003$), logrando un **$\text{SNR} > 75\text{ dB}$**.

### ¿Por qué la agitación fuerte (shaking) hace caer el SNR?
1. **Se supera el límite:** Al agitar la placa vigorosamente, la aceleración superó los $3.0g$. Con $\|a\| = 3.1g$:
   $$x_{\text{norm}} = \frac{3.1}{2.0} - 0.5 = +1.05 > 1.0$$
2. **Saturación forzada (*Clipping*):** Un entero Q15 no puede valer más de $+32767$ ($+1.0$). El valor de $+1.05$ se tiene que recortar obligatoriamente a $+1.0$.
3. **El error de distorsión se multiplica:** La diferencia entre el float real ($1.05$) y el reconstruido ($1.0$) es $0.05$ (más de 1600 veces mayor que el redondeo normal).
4. **Desplome del SNR:** Como $\text{SNR} = 10 \log_{10} (\text{Potencia Señal} / \text{Potencia Error})$, un error tan grande en el denominador hace que el SNR caiga en picado de **$80\text{ dB}$ a solo $46.0\text{ dB}$** (distorsión por saturación, como un micrófono al que se le grita de cerca).

---

## 5. Arquitectura Multihilo (Dual-Core RP2350)

```
       [ ICM-20948 IMU ]
              │ (I2C a 400 kHz)
              ▼
    ╔════════════════════════════╗
    ║ Core 1: Adquisición        ║  (Corre a 500 Hz exactos, cada 2000 µs)
    ╚════════════════════════════╝
              │
              ▼ (FIFO Queue protegida contra colisiones: sample_q)
    ╔════════════════════════════╗
    ║ Core 0: DSP y Almacenamiento║ (Llena buffer de 256 muestras, calcula
    ╚════════════════════════════╝  estadísticas, FFT, Q15 y guarda en SD)
              │
       ┌──────┴──────────────┐
       ▼                     ▼
 [ MicroSD SPI ]      [ USB Serial ]
 (Archivos CSV/BIN)   (Telemetría en vivo)
```

**¿Por qué es imprescindible usar los dos núcleos?**  
Escribir en la tarjeta MicroSD mediante SPI y FATFS tarda ocasionalmente entre **10 y 25 milisegundos** debido al borrado/escritura interno de bloques flash. Si Core 1 y Core 0 fueran un solo hilo, durante esos 20 ms el microcontrolador se congelaría y se perderían unas 10 muestras del sensor. Con dos núcleos, Core 1 sigue leyendo sin interrupción y depositando las muestras en la cola `sample_q`.

---

## 6. Cómo se Ejecuta el Código y qué Resultados Genera Cada Archivo

### A. Flujo de Compilación y Ejecución en el Embebido

```
  CMakeLists.txt + main.c + dsp_features.c
                     │
                     ▼ (Compilación con arm-none-eabi-gcc y Pico SDK)
             build/imu.uf2 (721 KB)
                     │
                     ▼ (Arrastrar a la unidad USB RPI-RP2 de la Pico 2)
        Ejecución en Raspberry Pi Pico 2 W
```

1. **Compilación en PC / Docker:**
   ```bash
   cd homework1
   mkdir -p build && cd build
   cmake ..
   make -j$(nproc)
   ```
   **Resultado:** Genera el archivo ejecutable `build/imu.uf2`.
2. **Flasheo en la placa:**
   - Mantener presionado el botón `BOOTSEL` de la Pico 2 y conectar el cable USB al PC.
   - Aparecerá la unidad de almacenamiento masivo `RPI-RP2`.
   - Copiar `imu.uf2` en la unidad. La placa se reiniciará automáticamente y comenzará a parpadear su LED.
3. **Telemetría en vivo por USB Serial:**
   - En Linux:
     ```bash
     cat /dev/ttyACM0
     # o bien con minicom:
     minicom -b 115200 -o -D /dev/ttyACM0
     ```
   - **Resultado:** Cada $\approx 512\text{ ms}$, Core 0 imprime en la terminal los resultados de la ventana:
     ```text
     [WINDOW #12 | t = 6.14 s | Rate = 500.0 Hz]
       Orientation / Mean (g) : ax=-0.012, ay=+0.045, az=+1.011
       Magnitude Stats (g)    : mean=1.012, std=0.0007, min=1.010, max=1.014, range=0.004, rms=1.012
       Spectral Features (FFT): Dom=1.95 Hz | Energy=7.46e-01 | Top Peaks:
         Peak 1: 1.95 Hz (amp = 0.8639 g)
       Q15 Quantization       : SNR=66.04 dB | RMS_err=0.000034 | Compression=2.0x
     ```

### B. Archivos Generados en la Tarjeta MicroSD
Tras cada experimento, la placa genera en la raíz de la tarjeta SD:
1. `imu_data.bin`: Archivo binario con todas las muestras crudas (`Sample`: timestamp de 64 bits, 3 aceleraciones int16, 3 giroscopios int16).
2. `imu_features.csv`: Archivo de texto estructurado con las 23 columnas de características calculadas ventana a ventana.

### C. Script de Verificación y Generación de Gráficos (`verify_homework1.py`)
Corre en el PC para validar matemáticamente los datos del microcontrolador y crear los gráficos del informe:
```bash
cd homework1
python3 verify_homework1.py
```
**Resultados generados:**
1. **Comprobación de paridad numérica:** Compara los valores del microcontrolador con los calculados independientemente en Python (NumPy) sobre las mismas muestras crudas. El error absoluto es inferior a $10^{-5}$ ($0.00001$), demostrando que la implementación embebida en C es matemáticamente exacta.
2. `figures/fig_time_freq_overview.png`: Muestra en una sola figura la aceleración triaxial $a_x, a_y, a_z$, la magnitud resultante $\|a\| \approx 1.0g$, y el espectro de frecuencia con los picos detectados.
3. `figures/fig_variations_overview.png`: Compara los tres experimentos físicos (rotación, tapping, shaking), mostrando la evolución de la desviación estándar $\sigma$ en el tiempo, la energía espectral y la caída de SNR en shaking.

### D. Informe Académico (`report.tex` -> `report.pdf`)
Compilación del informe formal de 3 páginas en formato IEEE:
```bash
cd homework1
pdflatex -interaction=nonstopmode report.tex
pdflatex -interaction=nonstopmode report.tex
```
**Resultado:** Genera `report.pdf`, listo para entregar, con exactamente 3 páginas, tablas comparativas y figuras de alta resolución.

---

## 7. Preguntas Típicas para Defender el Proyecto (Q&A)

### P1: ¿Por qué calculas la norma euclídea $\|a\|$ en vez de analizar sólo el eje $Z$?
**Respuesta:** Porque en un dispositivo que se mueve libremente en el espacio, la orientación cambia continuamente. La gravedad de $1g$ se proyecta en $X$, $Y$ o $Z$ según cómo esté inclinado. Al calcular $\|a\| = \sqrt{a_x^2 + a_y^2 + a_z^2}$, la magnitud es **invariante ante la rotación**: en reposo siempre vale $\approx 1.0g$, sin importar cómo coloques la placa.

### P2: ¿Por qué aplicas una ventana de Hamming antes de la FFT?
**Respuesta:** Porque la FFT asume que el bloque de 256 muestras se repite periódicamente en el tiempo. Si el inicio ($x_0$) y el final ($x_{255}$) tienen valores distintos, se produce una discontinuidad brusca en los bordes que crea ruido de falsas frecuencias en todo el espectro (**fuga espectral** o *spectral leakage*). La ventana de Hamming atenúa suavemente los extremos a cero, eliminando esa fuga sin deformar los picos centrales.

### P3: ¿Cuál es la resolución en frecuencia de tu FFT?
**Respuesta:** Como muestreamos a $f_s = 500\text{ Hz}$ con ventanas de $N = 256$ muestras, la resolución por cada bin es:
$$\Delta f = \frac{f_s}{N} = \frac{500}{256} \approx 1.953\text{ Hz}$$
Cada bin del espectro representa un múltiplo de $1.95\text{ Hz}$ ($0\text{ Hz}, 1.95\text{ Hz}, 3.91\text{ Hz}, 5.86\text{ Hz}, \dots$).

### P4: ¿Por qué el bin 0 (0 Hz) se ignora al buscar el pico dominante?
**Respuesta:** El bin 0 representa la componente continua (DC), que en un acelerómetro es la aceleración estática de la gravedad ($1.0g$). Si no lo descartáramos, el pico dominante siempre sería $0\text{ Hz}$, tapando cualquier movimiento real del usuario.

### P5: ¿Cómo garantizas que no se pierden muestras cuando la tarjeta SD tarda en escribir?
**Respuesta:** Usando los dos núcleos del microcontrolador RP2350. Core 1 corre un bucle de adquisición dedicado a 500 Hz que únicamente lee el sensor I2C y encola los datos. Core 0 se encarga de desencolar, procesar y escribir en la SD mediante FATFS. Si la SD tarda 15 ms en escribir un bloque, las muestras del sensor se acumulan de forma segura en la cola FIFO sin perderse ninguna.

### P6: ¿Qué ventaja ofrece la cuantización Q15?
**Respuesta:** Permite almacenar las muestras utilizando enteros de 16 bits (`int16_t`, 2 bytes) en lugar de números decimales de 32 bits (`float`, 4 bytes). Esto reduce el tráfico de escritura en la SD y el uso de memoria a la mitad (ahorro del 50%), manteniendo una calidad de señal muy alta ($\text{SNR} > 75\text{ dB}$ en condiciones normales).

### P7: ¿Por qué usamos una normalización fija $(a/2) - 0.5$ y no la fórmula típica de Min-Max $\frac{a - a_{\min}}{a_{\max} - a_{\min}}$?
**Respuesta:** Por tres razones técnicas fundamentales en sistemas embebidos:
1. **Evitar la amplificación masiva de ruido en reposo:** Si la placa está sobre la mesa quieta, la aceleración apenas varía entre $1.009g$ y $1.013g$ ($a_{\max} - a_{\min} \approx 0.004g$, mero ruido electrónico del sensor). Si aplicáramos Min-Max, ese ruido insignificante de $0.004g$ se estiraría a toda la escala $[-1.0, +1.0]$, haciendo creer erróneamente que la placa se sacude violentamente.
2. **Preservar el significado físico absoluto sin enviar metadatos:** Con una escala fija conocida por emisor y receptor, los números cuantizados mantienen siempre su valor físico real. Con Min-Max dinámico, cada ventana de 256 muestras tendría una escala distinta y estarías obligado a guardar o transmitir $a_{\min}$ y $a_{\max}$ como metadatos adicionales en cada bloque.
3. **Eficiencia en el microcontrolador y seguridad numérica:** Min-Max requiere hacer dos pasadas completas al buffer en memoria (una para buscar extremos y otra para normalizar) y una división de coma flotante por cada muestra, con el riesgo añadido de división por cero si $a_{\max} = a_{\min}$ (señal plana). La fórmula fija se ejecuta en una sola pasada en 1 ciclo de reloj con la FPU del Cortex-M33 (`a * 0.5f - 0.5f`).

---

## 8. Mapa de Código: Dónde Están las Partes Más Importantes y qué Hace Cada Una

Esta sección es tu "chuleta" rápida para saber exactamente en qué archivo y en qué línea está cada pieza clave del proyecto:

### 1. Bucle de Adquisición a 500 Hz en Core 1 (Tiempo Real)
- **Archivo:** [`homework1/main.c` (líneas 33-57)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/main.c#L33-L57)
- **Función:** `static void core1_reader(void)`
- **Código clave:**
  ```c
  absolute_time_t target = get_absolute_time();
  while (1) {
      target = delayed_by_us(target, 2000);  // 2000 us = 500 Hz
      imuDataAccGyrGet(&stGyroRawData, &stAccelRawData);
      Sample s = { ... };
      queue_add_blocking(&sample_q, &s);
      sleep_until(target);
  }
  ```
- **¿Qué hace?** Corre exclusivamente en el segundo núcleo del microcontrolador. Lee los registros I2C del sensor cada $2000\,\mu\text{s}$ exactos mediante un temporizador por hardware (`sleep_until`) y coloca la muestra en la cola segura `sample_q`. No hace cálculos pesados para no perder nunca el ritmo de 500 Hz.

---

### 2. Inicialización de Periféricos y Cola Inter-Núcleo
- **Archivo:** [`homework1/main.c` (líneas 121-136)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/main.c#L121-L136)
- **Código clave:**
  ```c
  queue_init(&sample_q, sizeof(Sample), 512); // Buffer circular de 512 muestras
  multicore_launch_core1(core1_reader);       // Arranca Core 1
  ```
- **¿Qué hace?** Reserva la cola de comunicación protegida contra condiciones de carrera (`mutex/spinlocks` internos del SDK) y arranca Core 1 para que empiece a muestrear en paralelo.

---

### 3. Conversión de Unidades Físicas y Magnitud Invariante
- **Archivo:** [`homework1/dsp_features.c` (líneas 11-28)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/dsp_features.c#L11-L28)
- **Función:** `imu_convert_units(...)`
- **Código clave:**
  ```c
  float ax = (float)raw_ax / ACCEL_SENSITIVITY_2G; // 16384.0f
  float ay = (float)raw_ay / ACCEL_SENSITIVITY_2G;
  float az = (float)raw_az / ACCEL_SENSITIVITY_2G;
  *amag_g = sqrtf(ax * ax + ay * ay + az * az);   // Magnitud ||a||
  *gx_dps = (float)raw_gx / GYRO_SENSITIVITY_1000DPS; // 32.8f
  ```
- **¿Qué hace?** Convierte las cuentas enteras crudas de 16 bits en gravedades físicas ($g$) y grados por segundo ($^\circ/\text{s}$). Calcula la magnitud euclídea $\|a\|$ para que la señal sea inmune a rotaciones e inclinaciones de la placa.

---

### 4. Estadísticas en el Dominio del Tiempo
- **Archivo:** [`homework1/dsp_features.c` (líneas 34-138)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/dsp_features.c#L34-L138)
- **Funciones:** `dsp_compute_stats()`, `dsp_mean_f32()`, `dsp_variance_f32()`, `dsp_median_f32()`, `dsp_rms_f32()`
- **Código clave:**
  ```c
  // En dsp_median_f32: Copia temporal para no alterar el orden del buffer
  for (int i = 1; i < n; i++) {
      float key = temp[i];
      int j = i - 1;
      while (j >= 0 && temp[j] > key) { temp[j + 1] = temp[j]; j--; }
      temp[j + 1] = key;
  }
  return (n % 2 == 1) ? temp[n/2] : 0.5f * (temp[n/2 - 1] + temp[n/2]);
  ```
- **¿Qué hace?** Extrae la media, varianza, desviación estándar, valores mínimo y máximo, rango dinámico, mediana y valor RMS sobre las 256 muestras de la ventana. Utiliza ordenamiento por inserción para la mediana sin destruir el vector original en el tiempo.

---

### 5. Ventana de Hamming y FFT Radix-2 Cooley-Tukey
- **Archivo:** [`homework1/dsp_features.c` (líneas 147-197 y 206-280)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/dsp_features.c#L147-L197)
- **Funciones:** `dsp_hamming_window()`, `dsp_fft_radix2()`, `dsp_extract_fft_features()`
- **Código clave:**
  ```c
  // 1. Ventana de Hamming para eliminar fugas espectrales
  dsp_hamming_window(win_buf, count);

  // 2. FFT en su lugar con inversión de bits y mariposas complejas
  dsp_fft_radix2(c_buf, count, +1);

  // 3. Escalado coherente y cálculo de energía (descartando bin 0)
  const float scale = (2.0f / (float)count) / 0.54f;
  for (int k = 1; k <= n_half; k++) energy += mag_raw[k] * mag_raw[k];

  // 4. Búsqueda de picos dominantes (ignora bin 0)
  for (int k = 1; k <= n_half; k++) { ... }
  ```
- **¿Qué hace?** Suprime discontinuidades en los bordes con Hamming, calcula el espectro de frecuencias con la FFT rápida de $O(N \log N)$, normaliza la ganancia y extrae la frecuencia dominante (ej: $1.95\text{ Hz}$ en tapping) y los 3 picos más altos.

---

### 6. Cuantización Q15 y Evaluación del SNR
- **Archivo:** [`homework1/dsp_features.c` (líneas 285-337)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/dsp_features.c#L285-L337)
- **Funciones:** `dsp_quantize_q15()`, `dsp_evaluate_quantization()`
- **Normalización en [`main.c:174`](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/main.c#L174):**
  ```c
  norm_amag[i] = (amag_arr[i] / 2.0f) - 0.5f; // Escala fija a [-1.0, 1.0)
  ```
- **Código clave de cuantización:**
  ```c
  if (x > 0.999969f)  { q = 32767;  (*clip_count)++; } // Clipping si a > 3.0g
  else if (x < -1.0f) { q = -32768; (*clip_count)++; }
  else { q = (int16_t)roundf(x * 32767.0f); }
  
  // Reconstrucción y SNR:
  float err = orig - recon;
  noise_pwr += err * err;
  eval->snr_db = 10.0f * log10f(sig_pwr / noise_pwr);
  ```
- **¿Qué hace?** Comprime cada muestra de 4 bytes (`float`) a 2 bytes (`int16_t`), detecta saturaciones cuando la aceleración pasa de $3.0g$, y calcula la relación señal/ruido en dB.

---

### 7. Escritura en Tarjeta SD y Telemetría Serial
- **Archivo:** [`homework1/main.c` (líneas 103-110 y 212-236)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/main.c#L103-L110)
- **Código clave:**
  ```c
  // Escritura binaria de datos crudos
  f_write(&raw_file, raw_window, sizeof(raw_window), &bw);
  f_sync(&raw_file);

  // Escritura de la fila con las 23 características calculadas
  int len = snprintf(csv_buf, sizeof(csv_buf),
      "%lu,%.3f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
      "%.2f,%.4e,%.2f,%.4f,%.2f,%.4f,%.2f,%.4f,%.2f,%.6f,%d\n", ...);
  f_write(&feat_file, csv_buf, len, &bw);
  f_sync(&feat_file);
  ```
- **¿Qué hace?** Vuelca la ventana cruda a `imu_data.bin` y la fila estructurada con las 23 características a `imu_features.csv` llamando a `f_sync()` para asegurar que se guarde en la memoria flash de la SD sin corromperse si se desconecta la alimentación.

---

### 8. Configuración de Hardware del Sensor ($\pm 2g$ y $\pm 1000^\circ/\text{s}$)
- **Archivo:** [`homework1/icm20948/icm20948.c` (líneas 242-250)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/icm20948/icm20948.c#L242-L250)
- **Código clave:**
  ```c
  I2C_WriteOneByte(REG_ADD_REG_BANK_SEL, REG_VAL_REG_BANK_2);
  I2C_WriteOneByte(REG_ADD_GYRO_CONFIG_1,
                   REG_VAL_BIT_GYRO_DLPCFG_6 | REG_VAL_BIT_GYRO_FS_1000DPS | REG_VAL_BIT_GYRO_DLPF);
  I2C_WriteOneByte(REG_ADD_ACCEL_CONFIG,
                   REG_VAL_BIT_ACCEL_DLPCFG_6 | REG_VAL_BIT_ACCEL_FS_2g | REG_VAL_BIT_ACCEL_DLPF);
  ```
- **¿Qué hace?** Selecciona el Bank 2 de registros del ICM-20948 mediante I2C y configura el rango de aceleración a $\pm 2g$ ($16384\text{ LSB}/g$) y el giroscopio a $\pm 1000^\circ/\text{s}$ ($32.8\text{ LSB}/(^\circ/\text{s})$), además de activar los filtros paso bajo digitales (DLPF) para mitigar el ruido electromagnético.

---

### 9. Script de Verificación y Generación de Figuras en PC
- **Archivo:** [`homework1/verify_homework1.py` (líneas 100-220)](file:///home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/verify_homework1.py#L100-L220)
- **Código clave:**
  ```python
  # Desempaqueta las estructuras binarias del archivo grabado en la Pico 2
  data = struct.unpack('<qhhhhhh', raw_bytes)
  # Recalcula con NumPy de forma independiente
  ref_mag = np.sqrt(ax**2 + ay**2 + az**2)
  ref_fft = np.abs(np.fft.rfft(ref_mag * np.hamming(256))) * (2.0 / 256.0) / 0.54
  # Compara MCU vs PC
  diff = abs(mcu_val - pc_val)
  assert diff < 1e-4
  ```
- **¿Qué hace?** Lee los datos reales grabados por la Pico 2 en tu SD (`imu_data.bin` e `imu_features.csv`), recalcula todas las funciones matemáticas en Python (NumPy) y genera los gráficos vectoriales para el informe (`fig_time_freq_overview.png` y `fig_variations_overview.png`).

