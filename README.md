# x64BareBones - Sistema Operativo

x64BareBones es un sistema operativo básico para arquitectura Intel 64 bits, con soporte completo para multiprocesamiento, sincronización y comunicación entre procesos.

## 🚀 Compilación y Ejecución

### Requisitos Previos

Instalar los siguientes paquetes antes de compilar:

```bash
nasm qemu gcc make
```

### Compilar con Docker (Recomendado)

```bash
./compilar.sh
```

Este script utiliza la imagen oficial: `docker pull agodio/itba-so-multi-platform:3.0`

### Compilar Manualmente

1. **Compilar el Toolchain:**
   ```bash
   cd Toolchain
   make all
   ```

2. **Compilar el proyecto completo:**
   ```bash
   make all
   ```

3. **Ejecutar el kernel:**
   ```bash
   ./run.sh
   ```

### Targets del Makefile

- `make all` - Compila bootloader, kernel, userland e imagen
- `make clean` - Limpia todos los archivos generados
- `make kernel` - Compila solo el kernel
- `make userland` - Compila solo el userland
- `make pvs` - Ejecuta análisis estático con PVS-Studio

---

## 🔍 Análisis Estático (PVS-Studio)

Este proyecto incluye integración completa con **PVS-Studio** para análisis estático de código.

### Requisitos Previos

#### En Linux/macOS (local)

Instalar PVS-Studio:

```bash
# Linux (Debian/Ubuntu)
wget -q -O - https://files.pvs-studio.com/etc/pubkey.txt | sudo apt-key add -
sudo wget -O /etc/apt/sources.list.d/viva64.list https://files.pvs-studio.com/etc/viva64.list
sudo apt-get update
sudo apt-get install pvs-studio

# Activar licencia (FREE para proyectos open source)
pvs-studio-analyzer credentials PVS-Studio FREE FREE
```

#### En Docker

El contenedor `agodio/itba-so-multi-platform:3.0` ya incluye las herramientas necesarias.

### Ejecutar Análisis

#### Análisis Completo

```bash
make pvs
```

Este comando ejecuta automáticamente:
1. Trace del build
2. Análisis estático
3. Generación de reporte HTML
4. Verificación de issues de alta severidad

#### Targets Individuales

```bash
# Solo trace del build
make pvs-trace

# Solo análisis (requiere trace previo)
make pvs-analyze

# Solo generar reporte HTML (requiere análisis previo)
make pvs-report

# Limpiar archivos de PVS
make pvs-clean
```

### Ver el Reporte

Después de ejecutar `make pvs`, abrir el reporte en un navegador:

```bash
# Linux
xdg-open pvs-report/index.html

# macOS
open pvs-report/index.html

# Windows (WSL)
explorer.exe pvs-report/index.html
```

### Exclusiones y Supresiones

#### Excluir Directorios

Para excluir directorios del análisis (ej. código de terceros):

```bash
./scripts/pvs.sh --exclude Bootloader/Pure64 --exclude Toolchain
```

#### Suprimir Falsos Positivos

Editar el archivo `.pvs/PVS-Studio.suppress` y agregar líneas en formato:

```
<archivo>:<línea>:<código_diagnóstico>
```

**Ejemplos:**

```
# Suprimir warning específico en un archivo
Kernel/lib.c:123:V522

# Suprimir todos los warnings de un directorio
Bootloader/Pure64/*:*:V::1042

# Suprimir un tipo de warning en todo el proyecto
*:*:V523
```

**Documentación de códigos de diagnóstico:** https://pvs-studio.com/en/docs/warnings/

### Interpretación de Severidades

PVS-Studio clasifica los issues en tres niveles:

- **GA:1 (High)** - Errores críticos, bugs probables
  - El pipeline de CI **falla** si encuentra issues GA:1
  - Deben ser corregidos o justificados antes de merge
  
- **GA:2 (Medium)** - Advertencias importantes, posibles bugs
  - Se reportan pero no fallan el CI
  - Revisar y corregir cuando sea posible
  
- **GA:3 (Low)** - Sugerencias de mejora de código
  - No se incluyen en el reporte por defecto

### CI/CD Integration

El proyecto incluye un workflow de GitHub Actions (`.github/workflows/pvs.yml`) que:

- ✅ Se ejecuta automáticamente en push y pull requests
- ✅ Genera reportes HTML como artifacts
- ✅ Falla si hay issues de severidad alta (GA:1)
- ✅ Sube reportes con retención de 30 días

**Ver reportes en GitHub Actions:**
1. Ir a la pestaña "Actions"
2. Seleccionar el workflow "PVS-Studio Static Analysis"
3. Descargar el artifact "pvs-studio-report"

### Política de Calidad

**Este proyecto se compila con `-Wall` sin warnings.**

Antes de hacer commit:
1. Asegurarse de que `make all` compila sin warnings
2. Ejecutar `make pvs` y verificar que no hay GA:1
3. Documentar cualquier supresión en `.pvs/PVS-Studio.suppress` con justificación

---

## 📚 Funcionalidades Implementadas

### Memory Management
- ✅ Buddy System allocator
- ✅ Syscalls: malloc, free, meminfo
- ✅ Test: test_mm

### Procesos y Scheduling
- ✅ Multitasking preemptivo
- ✅ Round-robin con prioridades (0-2)
- ✅ Context switching
- ✅ Aging anti-starvation
- ✅ Syscalls: create_process, kill, block, unblock, nice, yield, wait, getpid, ps
- ✅ Tests: test_processes, test_priority

### Sincronización (Semáforos)
- ✅ Semáforos compartibles entre procesos
- ✅ Sin busy waiting (usando instrucción atómica xchg)
- ✅ Syscalls: sem_create, sem_open, sem_close, sem_wait, sem_signal
- ✅ Tests: test_sync (resultado = 0), test_no_sync (resultado varía)

### IPC (Pipes)
- ✅ Pipes unidireccionales
- ✅ Read/write bloqueantes
- ✅ Procesos agnósticos (usan STDIN sin conocer origen)
- ✅ Compartibles entre procesos no relacionados
- ✅ Syscalls: pipe_create, pipe_open, pipe_close, pipe_read, pipe_write

### Comandos de Usuario
- ✅ Shell interactivo (proceso real)
- ✅ Soporte de pipes (`|`) y background (`&`)
- ✅ Comandos: help, mem, ps, loop, kill, nice, block, unblock, cat, wc, filter
- ✅ mvar N M (lectores/escritores con semáforos)

### Ejemplos de Uso

```bash
# Listar procesos
ps

# Proceso en loop
loop 2 &

# Usar pipes
ps | wc
ps | filter
loop 1 | cat

# Cambiar prioridad
nice <PID> 2

# Tests de sincronización
test_sync 100000 1
test_no_sync 100000 0

# Múltiples lectores/escritores
mvar 3 5
```

---

## 👥 Autores

- **Autor Original:** Rodrigo Rearden (RowDaBoat)
- **Colaborador:** Augusto Nizzo McIntosh
- **Grupo TP2-SO:** [Tu nombre/grupo]

---

## 📄 Licencia

Ver archivo `License.txt`
