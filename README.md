# A3 - Sistema de Tracking con Alerta de Hombre Caído

Sistema de tiempo real para detección y alerta de caídas humanas utilizando sensores IMU y comunicación LoRa.

## 📋 Descripción del Proyecto

Este proyecto implementa un sistema distribuido de detección de caídas en tiempo real que consta de:

- **Nodo móvil (portátil)**: Equipado con sensor IMU para detectar caídas
- **Nodo receptor**: Recibe y procesa alertas de caídas
- **Comunicación LoRa**: Enlace de radio de largo alcance para transmisión de alertas

## 🏗️ Arquitectura

El sistema está diseñado con una arquitectura por capas:

```
┌─────────────────────────────────────┐
│           Aplicación (App)          │  ← Orquestación RTOS
├─────────────────────────────────────┤
│          Servicios (Domain)         │  ← Lógica de negocio
├─────────────────────────────────────┤
│         Drivers (HAL)               │  ← Hardware abstraction
└─────────────────────────────────────┘
```

### Componentes principales:

- **Drivers**: `imu_accel`, `lora_radio`, `gpio_led`, `timers`, `watchdog`
- **Servicios**: `fall_detector`, `alert_queue`, `pkt_codec`
- **Aplicación**: Tareas RTOS con prioridades definidas

## 📁 Estructura del Proyecto

```
a3-hombre-caido/
├── docs/
│   └── A3_Arquitectura_Capas.md
├── firmware_node/
│   ├── README_drivers.md
│   ├── README_services.md
│   └── README_app.md
├── firmware_rx/
│   └── README_app_rx.md
├── config/
│   └── README_config.md
└── README.md
```

## ⚡ Características Técnicas

- **Detección en tiempo real**: Algoritmo de detección de picos + inmovilidad
- **Latencia crítica**: Confirmación → transmisión ≤ 300ms
- **Comunicación robusta**: Protocolo con CRC y timeouts
- **Arquitectura determinista**: Sin asignación dinámica en hot path

## 🚀 Próximos Pasos

1. ✅ Completar drivers `imu_accel` y `lora_radio` con timeouts
2. ✅ Implementar `fall_detector`, `alert_queue`, `pkt_codec`
3. ✅ Crear tareas y timers (100–200 Hz) y validar latencia
4. ✅ RX decodifica y muestra alertas

## 📚 Documentación

- [Arquitectura por Capas](docs/A3_Arquitectura_Capas.md) - Diseño detallado del sistema
- [Drivers](firmware_node/README_drivers.md) - Capa de abstracción de hardware
- [Servicios](firmware_node/README_services.md) - Lógica de dominio
- [Aplicación](firmware_node/README_app.md) - Orquestación RTOS
- [Receptor](firmware_rx/README_app_rx.md) - Nodo receptor
- [Configuración](config/README_config.md) - Parámetros del sistema

## 🔧 Requisitos

- FreeRTOS
- Hardware: IMU (acelerómetro), módulo LoRa
- Frecuencia de muestreo: 100-200 Hz
- Comunicación: LoRa 915MHz, SF7, BW125kHz

## 📄 Licencia

Este proyecto es parte del curso "Sistemas de Tiempo Real" - Universidad Nacional de Córdoba.
