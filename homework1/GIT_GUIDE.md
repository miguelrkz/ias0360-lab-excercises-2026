# Guía Completa de Comandos Git del Proyecto

Este documento explica paso a paso **todos los comandos de Git que se han utilizado**, **cómo se clonó y configuró el repositorio de entrega**, **cómo se configuró el usuario y la rama**, y **cómo puedes inspeccionar o subir los cambios cuando estés listo**.

---

## 1. Mapa de Directorios y Repositorios

En este proyecto tienes **dos carpetas principales**:

```
ias0360-lab-excercises-2026/          <-- Repositorio principal de prácticas / trabajo
├── homework1/                       <-- Tu carpeta de desarrollo local de la Tarea 1
│   ├── main.c
│   ├── dsp_features.c / .h
│   ├── report.tex / report.pdf
│   ├── verify_homework1.py
│   ├── EXPLANATION.md               <-- Tu guía local de estudio (NO se entrega)
│   ├── GIT_GUIDE.md                 <-- Esta guía de Git (NO se entrega)
│   └── data/                        <-- Datos reales de tus pruebas en la Pico 2
│
└── submission/                      <-- Carpeta donde vive el repositorio de entrega
    └── ias0360-home-assignment-1-submission-2026/   <-- REPO DE ENTREGA (GitHub)
        ├── main.c
        ├── dsp_features.c / .h
        ├── report.tex / report.pdf
        ├── verify_homework1.py
        └── ... (código limpio listo para el profesor)
```

---

## 2. Cómo se Clonó el Repositorio de Entrega

El repositorio de entrega de la asignatura es:
`git@github.com:taltech-eailab-courses/ias0360-home-assignment-1-submission-2026.git`

Para clonarlo dentro de la carpeta `submission/`:

```bash
# 1. Crear carpeta submission si no existía y entrar en ella
mkdir -p submission
cd submission

# 2. Clonar el repositorio usando SSH (o HTTPS)
git clone git@github.com:taltech-eailab-courses/ias0360-home-assignment-1-submission-2026.git

# 3. Entrar a la carpeta del repositorio clonado
cd ias0360-home-assignment-1-submission-2026
```

> **¿SSH o HTTPS?**  
> Al clonar con `git@github.com:...`, Git usa tu clave SSH configurada en tu máquina (`~/.ssh/id_ed25519` o `~/.ssh/id_rsa`). Si se usa HTTPS (`https://github.com/...`), GitHub solicita un Personal Access Token (PAT).

---

## 3. Configuración del Usuario (Nombre y Correo)

Para que los commits lleven tu nombre y correo oficial de estudiante:

```bash
# Entrar al repositorio de entrega
cd /home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/submission/ias0360-home-assignment-1-submission-2026

# Configurar nombre de autor
git config user.name "Miguel Robledo"

# Configurar correo del autor
git config user.email "miguel.robledo@upc.edu"
```

### ¿Qué hace `--global` vs sin `--global`?
- `git config --global user.name "..."`: Configura el nombre para **todos** los repositorios Git de tu usuario en Linux (`~/.gitconfig`).
- `git config user.name "..."` (sin `--global`): Configura el nombre **únicamente para este repositorio local** (`.git/config`), sobrescribiendo cualquier configuración global.

Para comprobar cómo está configurado actualmente:
```bash
git config -l
```

---

## 4. Creación y Formato de la Rama del Alumno (`submissions/miguro`)

El enunciado del Homework exige expresamente:
> *"Create your own branch in the following format: `submissions/[student code]`"*

Donde `[student code]` es tu código o identificador de estudiante de TalTech (tu **UNI-ID**: `miguro`). Por lo tanto, el nombre exacto de la rama debe ser:
`submissions/miguro`

### Cómo se crea directamente:
```bash
git checkout -b submissions/miguro
```

### O cómo se renombra una rama existente:
Si inicialmente se había creado como `miguro`, para renombrarla al formato oficial requerido:
```bash
git branch -m submissions/miguro
```

Para verificar en qué rama estás:
```bash
git branch
# La rama con un asterisco (*) verde es la activa:
# * submissions/miguro
#   main
```

---

## 5. Sincronización de Archivos desde `homework1/`

El desarrollo se realiza en `homework1/` (donde compilas, ejecutas y generas los datos y el PDF). Cuando los archivos están listos, se sincronizan al repositorio de entrega:

```bash
# Copiar el código fuente, librerías, drivers, scripts, datos y el informe
cp -r /home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/homework1/* \
      /home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/submission/ias0360-home-assignment-1-submission-2026/

# Si se coló algún archivo de estudio como EXPLANATION.md o GIT_GUIDE.md, se borra de la entrega:
rm -f /home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/submission/ias0360-home-assignment-1-submission-2026/EXPLANATION.md
rm -f /home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/submission/ias0360-home-assignment-1-submission-2026/GIT_GUIDE.md
```

---

## 6. Comandos para Preparar y Crear Commits

### Paso A: Ver qué archivos han cambiado (`git status`)
```bash
git status
```
Muestra:
- En **verde**: archivos en el *Staging Area* (listos para el commit).
- En **rojo**: archivos modificados o nuevos (*untracked*) aún no añadidos.

### Paso B: Añadir archivos al Staging Area (`git add`)
```bash
# Añadir archivos específicos:
git add report.tex report.pdf README.md

# O añadir todos los cambios y archivos nuevos del repositorio:
git add -A
```

### Paso C: Crear el Commit (`git commit`)
```bash
git commit -m "Implement Homework 1: Real-Time IMU Feature Extraction, DSP & Report (miguro)"
```
Guarda una instantánea fija de los archivos preparados en el historial de la rama `miguro`.

---

## 7. Cómo Modificamos Commits sin Crear Basura (`git commit --amend`)

Durante el trabajo, cuando simplificamos el informe `report.tex`, añadimos la explicación de $\pm 2g$ o quitamos `EXPLANATION.md`, **no creamos 10 commits desordenados**, sino que usamos:

```bash
git commit --amend -m "Nuevo mensaje de commit"
```

### ¿Qué hace `--amend`?
- **Corrige o sustituye el último commit**.
- Fusiona los nuevos cambios que hayas añadido con `git add` directamente dentro del commit anterior, manteniendo un historial limpio y profesional (como si lo hubieras hecho perfecto a la primera).
- Evita dejar rastros de archivos que borraste (como `EXPLANATION.md`).

---

## 8. Comandos de Inspección y Verificación

Estos son los comandos clave para revisar que todo esté perfecto:

### 1. Ver el historial de commits simplificado
```bash
git log --oneline -n 5
```
*Salida actual en tu repo:*
```text
2f80c2b (HEAD -> miguro) Add explanation of LSB/g, division factors 16384 and 32.8, and physical units in report
b859f3c Implement Homework 1: Real-Time IMU Feature Extraction, DSP & Report (miguro)
```

### 2. Ver exactamente qué archivos se modificaron en el último commit
```bash
git log -n 1 --stat
```
*Muestra cuántas líneas se insertaron y borraron en `report.tex` y el cambio binario en `report.pdf`.*

### 3. Ver todos los archivos controlados por Git (para verificar que no haya archivos extra)
```bash
git ls-files
```
Para comprobar específicamente que `EXPLANATION.md` no está:
```bash
git ls-files | grep -i explanation
# Si no devuelve nada, ¡está 100% limpio!
```

### 4. Ver qué cambios exactos hay sin commitear (diferencias de código)
```bash
git diff
```

---

## 9. El Paso Final: Cómo Subir la Solución a GitHub (`git push`)

**Importante:** Tal como pediste (*"no, do not push it yet"*), **aún no se ha hecho ningún push** a los servidores remotos de GitHub. Todos los commits están a salvo en tu máquina local.

Cuando hayas revisado todo y decidas que es el momento de entregar tu tarea:

```bash
# 1. Ve al directorio del repositorio de entrega
cd /home/miguelrkz/Documents/TALTECH/ias0360-lab-excercises-2026/submission/ias0360-home-assignment-1-submission-2026

# 2. Asegúrate de estar en la rama correcta
git branch
# Debe mostrar: * submissions/miguro

# 3. Sube tu rama al servidor remoto (origin)
git push -u origin submissions/miguro
```

### ¿Qué significa `-u origin submissions/miguro`?
- `origin`: Es el alias del repositorio remoto de GitHub (`git@github.com:taltech-eailab-courses/...`).
- `submissions/miguro`: El nombre de la rama oficial de entrega.
- `-u` (o `--set-upstream`): Vincula tu rama local con la remota, para que en el futuro solo tengas que escribir `git push` o `git pull`.

---

## 10. Resumen Rápido (Cheat Sheet de Comandos)

| Comando | Para qué sirve |
| :--- | :--- |
| `git clone <url>` | Descarga una copia del repositorio remoto en tu máquina. |
| `git config user.name "Tu Nombre"` | Define quién eres como autor de los commits. |
| `git checkout -b <rama>` | Crea una rama nueva y se cambia a ella. |
| `git status` | Muestra qué archivos están modificados, añadidos o sin seguir. |
| `git add <archivo>` | Prepara un archivo para incluirlo en el próximo commit. |
| `git commit -m "mensaje"` | Guarda permanentemente los cambios preparados con una descripción. |
| `git commit --amend` | Modifica o actualiza el último commit sin crear uno nuevo. |
| `git log --oneline` | Muestra el historial de commits en una sola línea por commit. |
| `git ls-files` | Lista todos los archivos que están siendo rastreados por Git. |
| `git push -u origin <rama>` | Publica tu rama local y sus commits en GitHub. |
