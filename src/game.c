#include <SDL2/SDL.h>
#include "game.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Tetromino shape definitions [type][rotation][y][x]
// 0: None, 1: I, 2: J, 3: L, 4: O, 5: S, 6: T, 7: Z
static const int TETROMINOES[8][4][4][4] = {
    // 0: None
    { {{0}} },

    // 1: I
    {
        { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
        { {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0} },
        { {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }
    },

    // 2: J
    {
        { {1,0,0}, {1,1,1}, {0,0,0} },
        { {0,1,1}, {0,1,0}, {0,1,0} },
        { {0,0,0}, {1,1,1}, {0,0,1} },
        { {0,1,0}, {0,1,0}, {1,1,0} }
    },

    // 3: L
    {
        { {0,0,1}, {1,1,1}, {0,0,0} },
        { {0,1,0}, {0,1,0}, {0,1,1} },
        { {0,0,0}, {1,1,1}, {1,0,0} },
        { {1,1,0}, {0,1,0}, {0,1,0} }
    },

    // 4: O
    {
        { {1,1}, {1,1} },
        { {1,1}, {1,1} },
        { {1,1}, {1,1} },
        { {1,1}, {1,1} }
    },

    // 5: S
    {
        { {0,1,1}, {1,1,0}, {0,0,0} },
        { {0,1,0}, {0,1,1}, {0,0,1} },
        { {0,0,0}, {0,1,1}, {1,1,0} },
        { {1,0,0}, {1,1,0}, {0,1,0} }
    },

    // 6: T
    {
        { {0,1,0}, {1,1,1}, {0,0,0} },
        { {0,1,0}, {0,1,1}, {0,1,0} },
        { {0,0,0}, {1,1,1}, {0,1,0} },
        { {0,1,0}, {1,1,0}, {0,1,0} }
    },

    // 7: Z
    {
        { {1,1,0}, {0,1,1}, {0,0,0} },
        { {0,0,1}, {0,1,1}, {0,1,0} },
        { {0,0,0}, {1,1,0}, {0,1,1} },
        { {0,1,0}, {1,1,0}, {1,0,0} }
    }
};

// Piece bounding box size
static const int PIECE_SIZE[8] = { 0, 4, 3, 3, 2, 3, 3, 3 };

// SRS Wall-kick tables (JLSTZ)
static const int KICKS_JLSTZ[8][5][2] = {
    // 0->1
    { {0,0}, {-1,0}, {-1, 1}, {0,-2}, {-1,-2} },
    // 1->0
    { {0,0}, { 1,0}, { 1,-1}, {0, 2}, { 1, 2} },
    // 1->2
    { {0,0}, { 1,0}, { 1,-1}, {0, 2}, { 1, 2} },
    // 2->1
    { {0,0}, {-1,0}, {-1, 1}, {0,-2}, {-1,-2} },
    // 2->3
    { {0,0}, { 1,0}, { 1, 1}, {0,-2}, { 1,-2} },
    // 3->2
    { {0,0}, {-1,0}, {-1,-1}, {0, 2}, {-1, 2} },
    // 3->0
    { {0,0}, {-1,0}, {-1,-1}, {0, 2}, {-1, 2} },
    // 0->3
    { {0,0}, { 1,0}, { 1, 1}, {0,-2}, { 1,-2} }
};

// SRS Wall-kick tables (I Piece)
static const int KICKS_I[8][5][2] = {
    // 0->1
    { {0,0}, {-2,0}, { 1,0}, {-2,-1}, { 1, 2} },
    // 1->0
    { {0,0}, { 2,0}, {-1,0}, { 2, 1}, {-1,-2} },
    // 1->2
    { {0,0}, {-1,0}, { 2,0}, {-1, 2}, { 2,-1} },
    // 2->1
    { {0,0}, { 1,0}, {-2,0}, { 1,-2}, {-2, 1} },
    // 2->3
    { {0,0}, { 2,0}, {-1,0}, { 2, 1}, {-1,-2} },
    // 3->2
    { {0,0}, {-2,0}, { 1,0}, {-2,-1}, { 1, 2} },
    // 3->0
    { {0,0}, { 1,0}, {-2,0}, { 1,-2}, {-2, 1} },
    // 0->3
    { {0,0}, {-1,0}, { 2,0}, {-1, 2}, { 2,-1} }
};

static const char *HOSPITAL_DEPARTMENTS[] = {
    "Urgencias y Triaje",
    "Consultas y Cita Previa",
    "Hospitalizacion y Planta",
    "Quirofanos y URPA",
    "Cuidados Intensivos (UCI)",
    "Radiologia, TAC y PACS",
    "Laboratorio y Farmacia",
    "Historia Clinica (HIS)",
    "Centro de Datos (CPD)",
    "Guardia Nocturna 24H",
    "Maestro de Sistemas TIC"
};

// 4 Integrantes de Guardia TIC en Hospital de Sant Joan
static const char *GUARDIAS[4] = {
    "Jose Maria",
    "Marichu",
    "Diego",
    "Ernesto"
};

const char *game_get_guardia_name(int idx) {
    if (idx < 0) idx = 0;
    return GUARDIAS[idx % 4];
}

int game_get_current_guardia_index(const Game *g) {
    int idx = (g->pieces_dropped / 25) % 4;
    if (idx < 0) idx = 0;
    return idx;
}

const char *game_get_current_guardia(const Game *g) {
    return game_get_guardia_name(game_get_current_guardia_index(g));
}

static const char *GUARDIA_TICKER_TEMPLATES[] = {
    "[URGENCIAS 02:45] %s:\n > Descubrio el cable en 5 segundos.",
    "[PLANTA 4 03:10] %s:\n > Arreglo la impresora de golpe.",
    "[CPD SOTANO 1] %s:\n > 'Hay cafe, la red esta a salvo.'",
    "[QUIROFANO 03:35] %s:\n > Anestesista con pantalla al reves.",
    "[CONSULTAS 04:20] %s:\n > Enchufo la regleta que apagaron.",
    "[TRIAJE 04:50] %s:\n > 'La de admision te guarda comida.'",
    "[FARMACIA 05:15] %s:\n > Desbloqueo 120 recetas del HIS.",
    "[DIRECCION 05:40] %s:\n > '\u00bfPodrias sintonizar la tele?'",
    "[UCI 06:05] %s:\n > Evito apagar el servidor central.",
    "[SISTEMAS 06:30] %s:\n > Script nocturno sin un solo fallo.",
    "[URGENCIAS 06:50] %s:\n > '\u00a1Nos has salvado la guardia!'",
    "[RADIOLOGIA 03:55] %s:\n > 'El TAC funciona pero hace ruido.'",
    "[LABORATORIO 04:35] %s:\n > La centrifuga no tiene Bluetooth.",
    "[CONSULTAS 05:55] %s:\n > '\u00bfSi borro el icono borro todo?'",
    "[SOPORTE 03:20] %s:\n > '\u00bfApagar y encender sirve?' (Si).",
    "[REHAB 02:30] %s:\n > El raton tenia la tapa puesta.",
    "[DIALISIS 03:40] %s:\n > Cable de red pisado por camilla.",
    "[CPD SOTANO 1] %s:\n > Cafe caliente para aguantar turno.",
    "[PLANTA 2 04:15] %s:\n > Medico con Bloq Mayus activado.",
    "[ARCHIVOS 04:45] %s:\n > El informe estaba en Descargas.",
    "[ADMISION 05:10] %s:\n > Impresora sin papel en bandeja.",
    "[QUIROFANO 05:35] %s:\n > Cable HDMI flojo en pantalla.",
    "[UCI 05:50] %s:\n > Sensor optico tapado con celo.",
    "[URGENCIAS 06:15] %s:\n > 'Doctor, encienda la pantalla.'",
    "[FARMACIA 06:35] %s:\n > Cola de impresion desbloqueada.",
    "[ANATOMIA 02:15] %s:\n > 'El microfono no tiene sonido.'",
    "[PLANTA 1 02:50] %s:\n > Switch de planta operativo 100%%.",
    "[PLANTA 3 03:25] %s:\n > Teclado con cafe desatascado.",
    "[PLANTA 5 04:05] %s:\n > Cable de red en clavija correcta.",
    "[CPD SOTANO 1] %s:\n > Cero incidencias criticas hoy.",
    "[SISTEMAS 04:30] %s:\n > Backup de datos completado OK.",
    "[CONSULTAS 04:55] %s:\n > '\u00bfDonde esta el boton de enviar?'",
    "[TRIAJE 05:25] %s:\n > Impresora lista para el turno.",
    "[DIRECCION 06:20] %s:\n > Felicitacion del jefe de servicio.",
    "[URGENCIAS 06:40] %s:\n > Aplausos en el cambio de turno.",
};

static const char *GENERAL_TICKER_POOL[] = {
    "[URGENCIAS 01:10] Raton bloqueado:\n > Estaba encima de un bocadillo.",
    "[PLANTA 3 01:18] Pantalla negra:\n > Solucion: Encender la regleta.",
    "[CONSULTAS 01:25] 'Borre Internet':\n > Solo cerro la pestana de Chrome.",
    "[QUIROFANO 01:32] Puntero invertido:\n > Alfombrilla y raton al reves.",
    "[TRIAJE 01:40] Impresora atascada:\n > Metieron radiografia en bandeja.",
    "[SOPORTE 01:48] '\u00bfLo reinicio?':\n > Funciono. Doctor anonadado.",
    "[FARMACIA 01:55] Password erroneo:\n > Bloqueo mayusculas activado.",
    "[CPD SOTANO 1] 16 grados en sala:\n > Informatico con forro polar.",
    "[UCI 02:02] Ticket P1 urgente:\n > Quitar pegatina del sensor.",
    "[RADIOLOGIA 02:12] TAC desconectado:\n > Desenchufado para cargar movil.",
    "[CAFETERIA 02:20] Alerta en el CPD:\n > 'Error 404: Cafe Not Found.'",
    "[CONSULTAS 02:28] PC apagado:\n > Tiraron del cable con la silla.",
    "[SISTEMAS 02:36] Switch perimetral:\n > Uptime de red al 100.00%%.",
    "[ALERTA 02:44] Silencio en CPD:\n > 'Si no tocas nada, aguantamos.'",
    "[TRIAJE 02:52] Cola de impresion:\n > 50 etiquetas de golpe listas.",
    "[DIRECCION 03:02] Ticket de guardia:\n > '\u00bfMe instalais el Solitario?'",
    "[PLANTA 2 03:08] PC no arranca:\n > Estaba enchufado a si mismo.",
    "[SOPORTE 03:16] Teclado gritando:\n > Escribia en MAYUSCULAS todo.",
    "[CPD SOTANO 1] Ruido extrano:\n > Era la cafetera pidiendo agua.",
    "[TRIAJE 03:24] Pantalla apagada:\n > Limpieza desenchufo el SAI.",
    "[CONSULTAS 03:32] Tecla Intro fija:\n > Cafe derramado sobre teclado.",
    "[RAYOS X 03:38] 'Red muy lenta':\n > Descargando pelicula en planta.",
    "[QUIROFANO 03:46] Alarma roja en PC:\n > Solo era actualizacion de Java.",
    "[UCI 03:54] Aviso de madrugada:\n > '\u00bfDonde se le da a guardar?'",
    "[PLANTA 5 04:02] Sin conexion a red:\n > Clavija de telefono en el RJ45.",
    "[LABORATORIO 04:08] Centrifugadora:\n > 'No imprime'. (No es impresora).",
    "[SISTEMAS 04:16] Copia en cintas:\n > Guardadas en caja ignifuga.",
    "[URGENCIAS 04:24] Impresora humeando:\n > Metieron 3 paquetes de folios.",
    "[ADMISION 04:32] 'Se cerro el HIS':\n > Clicaron en la X roja superior.",
    "[SOPORTE 04:38] Llamada del 112:\n > 'Se ha caido un cuadro en sala.'",
    "[CONSULTAS 04:46] 'Hay un virus':\n > Era el salvapantallas de peces.",
    "[REHAB 04:52] Raton no desliza:\n > Plastico protector sin quitar.",
    "[FARMACIA 04:58] Lector optico:\n > Escaneando el codigo al reves.",
    "[DIRECCION 05:04] 'Fallo critico':\n > Olvido su PIN de 4 cifras.",
    "[ANATOMIA 05:12] Peticion al CPD:\n > 'Queremos altavoces con bajos.'",
    "[URGENCIAS 05:18] 'Sin sonido en PC':\n > Auriculares en toma microfono.",
    "[SISTEMAS 05:26] Firewall perimetral:\n > Bloqueada estafa del principe.",
    "[QUIROFANO 05:32] 'Se ve todo azul':\n > Filtro de luz nocturna puesto.",
    "[PLANTA 1 05:38] Monitor parpadea:\n > Cable VGA suelto y colgando.",
    "[TRIAJE 05:44] 'No entra tarjeta':\n > Metian la del bonobus urbano.",
    "[CPD SOTANO 1] Beep de alarma:\n > Era el microondas con la leche.",
    "[SOPORTE 05:52] 'No toque nada':\n > Toco todos los cables de abajo.",
    "[URGENCIAS 05:58] Atasco de papel:\n > '\u00bfHabia que quitar las grapas?'",
    "[CONSULTAS 06:04] 'Se fue la luz':\n > Dio al interruptor con el pie.",
    "[PLANTA 3 06:12] Ventilador ruidoso:\n > Un folio doblado en la rejilla.",
    "[SISTEMAS 06:18] Fibra optica:\n > Latencia media 1 milisegundo.",
    "[URGENCIAS 06:26] Pulsera atascada:\n > Pegatina pegada en el rodillo.",
    "[ARCHIVOS 06:34] 'Perdi la carpeta':\n > Estaba dentro de la Papelera.",
    "[QUIROFANO 06:42] 'Huele a tostada':\n > Tostada quemada en descanso.",
    "[UCI 06:48] 'El Wi-Fi se corta':\n > Estaban en la sala plomada.",
    "[CPD SOTANO 1] Relevo proximo:\n > Ya se huelen churros y porras.",
    "[PLANTA 4 01:15] Cuna termica TIC:\n > Sensor de red conectado al SAI.",
    "[URGENCIAS 01:28] 'El teclado baila':\n > Le faltaba una patilla de goma.",
    "[CONSULTAS 01:42] Mensaje de error:\n > 'Pulse cualquier tecla'. Dudan.",
    "[LABORATORIO 01:52] Tubos de ensayo:\n > Codigo de barras doblado.",
    "[QUIROFANO 02:05] Lampara quirurgica:\n > No lleva Wi-Fi, es mecanica.",
    "[SOPORTE 02:18] Llamada a las 02 AM:\n > '\u00bfTeneis grapas en el CPD?'",
    "[FARMACIA 02:32] Nevera de vacunas:\n > Sensor de temperatura: 4C OK.",
    "[CPD SOTANO 1] Luces verdes rack:\n > El espectaculo visual relaja.",
    "[TRIAJE 02:48] Pantalla tactil:\n > Limpiada con alcohol, resucito.",
    "[PLANTA 1 03:05] Cama con motor:\n > 'No sincroniza con el PC.'",
    "[DIRECCION 03:15] Impresora color:\n > Gastaron el magenta en fotos.",
    "[RAYOS X 03:30] Consola de mando:\n > Monitor secundario apagado.",
    "[UCI 03:42] Monitor signos:\n > Cable ethernet bien engastado.",
    "[PLANTA 2 03:55] Silla de ruedas:\n > Se engancho al cable de red.",
    "[CONSULTAS 04:12] 'No sale la firma':\n > Tableta digitalizadora al reves.",
    "[SISTEMAS 04:20] Enlace troncal:\n > 10 Gigabits por segundo OK.",
    "[URGENCIAS 04:35] PC va muy lento:\n > 48 pestanas abiertas en Chrome.",
    "[ARCHIVOS 04:48] Lector microfilm:\n > Enchufado a toma sin corriente.",
    "[QUIROFANO 05:02] Aspirador:\n > No es un dispositivo USB.",
    "[FARMACIA 05:14] Robot dispensador:\n > Brazo mecanico calibrado.",
    "[PLANTA 3 05:28] Timbre de cama:\n > 'Creiamos que era el raton.'",
    "[SOPORTE 05:42] Ticket misterioso:\n > Resuelto antes de ir al box.",
    "[TRIAJE 05:55] Teclado lavable:\n > Sumergido en agua con jabon.",
    "[CPD SOTANO 1] Monitor de red:\n > Graficas en verde absoluto.",
    "[DIALISIS 01:20] 'No carga sesion':\n > No habian iniciado sesion.",
    "[PLANTA 5 01:38] 'El cursor tiembla':\n > El escritorio cojeaba.",
    "[REHAB 01:50] Cinta de correr:\n > '\u00bfSe puede conectar al HIS?'",
    "[ANATOMIA 02:10] Camara microscopio:\n > Tapa del objetivo puesta.",
    "[URGENCIAS 02:22] Box de paradas:\n > Terminal ligero listo y rapido.",
    "[CONSULTAS 02:40] Lector DNIe:\n > Tarjeta insertada al reves.",
    "[QUIROFANO 02:58] Torre laparoscopia:\n > Canal de video bien conmutado.",
    "[SISTEMAS 03:12] Dominio Windows:\n > Servidor LDAP respondiendo 2ms.",
    "[UCI 03:35] Bomba de infusion:\n > Transmision de datos perfecta.",
    "[FARMACIA 03:48] Cajon automatico:\n > Moneda atascada en el carril.",
    "[PLANTA 2 04:02] 'La pantalla vibra':\n > Ventilador portatil muy cerca.",
    "[TRIAJE 04:18] Puesto 2 de triaje:\n > Reemplazado cable defectuoso.",
    "[CPD SOTANO 1] Suelo tecnico:\n > Ningun cable mordido ni roto.",
    "[RAYOS X 04:32] Resonancia magnetica:\n > Cero interferencias en red.",
    "[DIRECCION 04:45] Portatil gerencia:\n > Interruptor Wi-Fi apagado.",
    "[URGENCIAS 05:00] Impresora laser:\n > Folios reciclados con grapas.",
    "[PLANTA 1 05:15] 'No oigo audio':\n > Auricular con volumen a cero.",
    "[SOPORTE 05:30] 'Tengo un virus':\n > Solo era un aviso de cookies.",
    "[QUIROFANO 05:48] Pantalla de signos:\n > Brillo al minimo por error.",
    "[ARCHIVOS 06:02] Escaner historias:\n > Cristal limpiado de pegamento.",
    "[PLANTA 4 06:15] Sala de lactancia:\n > Punto de acceso Wi-Fi al 100%%.",
    "[LABORATORIO 06:30] Nevera muestras:\n > Telemetria de grados enviada.",
    "[SISTEMAS 06:45] Certificados SSL:\n > Renovados a tiempo y validados.",
    "[CPD SOTANO 1] Cafe de las 06:30:\n > La cafetera merece una estatua.",
    "[URGENCIAS 06:55] Cambio de guardia:\n > Todos los puestos operativos.",
    "[CONSULTAS 01:12] Doble clic veloz:\n > Hacia triple clic y cerraba.",
    "[PLANTA 3 01:30] 'No veo la barra':\n > Barra de tareas auto-oculta.",
    "[QUIROFANO 01:45] Pedal de electro:\n > Confundido con raton de pie.",
    "[SOPORTE 02:00] Doctor a las 2 AM:\n > '\u00bfCual era mi contrasena?'",
    "[TRIAJE 02:15] 'Faltan folios':\n > Estaban en el cajon de abajo.",
    "[FARMACIA 02:30] Bote de jarabe:\n > Derramado al lado del raton.",
    "[CPD SOTANO 1] Termometro CPD:\n > Aire acondicionado a tope.",
    "[UCI 02:45] Marcapasos monitor:\n > Senal transmitida sin cortes.",
    "[RADIOLOGIA 03:00] Pantalla DICOM:\n > Calibracion de grises exacta.",
    "[PLANTA 2 03:18] Toma de corriente:\n > Cargador de cepillo electrico.",
    "[CONSULTAS 03:30] 'Perdi la cita':\n > Habia minimizado la ventana.",
    "[URGENCIAS 03:45] Lector de tarjetas:\n > Polvo soplado, lector listo.",
    "[SISTEMAS 04:00] Router de salida:\n > BGP estable con Conselleria.",
    "[PLANTA 5 04:15] 'No puedo imprimir':\n > Impresora seleccionada: PDF.",
    "[LABORATORIO 04:30] Tubo neumatico:\n > El envio llego a destino OK.",
    "[DIRECCION 04:45] Firma digital:\n > Tarjeta criptografica leida.",
    "[QUIROFANO 05:00] Monitor anestesia:\n > Reseteado con boton trasero.",
    "[REHAB 05:15] Aparato de laser:\n > 'No emite Wi-Fi'. (Normal).",
    "[TRIAJE 05:30] Impresora turnos:\n > Rollo termico colocado bien.",
    "[ARCHIVOS 05:45] Estanteria rodante:\n > No atranco ningun cable TIC.",
    "[PLANTA 1 06:00] Olor a tostada:\n > Falsa alarma en office medico.",
    "[SOPORTE 06:15] '\u00bfPuedo apagarlo?':\n > 'No doctor, dejelo encendido.'",
    "[URGENCIAS 06:30] Telefono guardia:\n > Silencio bendito por 5 minutos.",
    "[CPD SOTANO 1] SAI en bypass:\n > Vuelve a red normal limpia.",
    "[SISTEMAS 06:45] Logs del sistema:\n > Cero accesos sospechosos.",
    "[DIALISIS 06:50] Monitor osmosis:\n > Parametros en rango verde.",
    "[PLANTA 4 01:22] Cuna con bascula:\n > Peso enviado por puerto COM.",
    "[URGENCIAS 01:35] 'No veo paciente':\n > Filtro en 'Dados de alta.'",
    "[CONSULTAS 01:58] Boligrafo caido:\n > Bloqueaba apertura de lector.",
    "[QUIROFANO 02:25] Cable de tierra:\n > Puesto a masa correctamente.",
    "[FARMACIA 02:42] Pistola de codigo:\n > Bateria cargada en la cuna.",
    "[CPD SOTANO 1] Monitor central:\n > Mapa de red en verde completo.",
    "[UCI 03:05] Desfibrilador:\n > Test automatico nocturno OK.",
    "[RADIOLOGIA 03:22] Chasis de rayos:\n > Bateria cambiada a tiempo.",
    "[PLANTA 3 03:40] 'La rueda no baja':\n > Scroll del raton limpiado.",
    "[TRIAJE 03:58] Monitor colgado:\n > Soporte VESA bien apretado.",
    "[SOPORTE 04:10] Medico temeroso:\n > '\u00bfSe borra si pulso Escape?'",
    "[SISTEMAS 04:28] Switch PoE planta:\n > Alimentando telefonos IP OK.",
    "[PLANTA 2 04:42] 'No oigo el timbre':\n > Altavoces apagados con rueda.",
    "[LABORATORIO 05:00] Contador celulas:\n > Puerto serie calibrado.",
    "[DIRECCION 05:18] Correo no llega:\n > Se auto-envio el correo a si.",
    "[QUIROFANO 05:35] Endoscopia:\n > Conector de fibra optica limpio.",
    "[ARCHIVOS 05:50] Lector de barras:\n > Cable estirado sin nudos.",
    "[URGENCIAS 06:05] Etiquetadora:\n > Sin pegamento en el rodillo.",
    "[PLANTA 5 06:20] 'El cursor salta':\n > Pelo de bata en sensor optico.",
    "[CPD SOTANO 1] Botellero de agua:\n > Bombona cambiada en el office.",
    "[SISTEMAS 06:35] Base de datos SQL:\n > Mantenimiento nocturno OK.",
    "[CONSULTAS 06:48] Silla ergonomica:\n > No se ha tragado ningun cable.",
    "[TRIAJE 06:58] Impresora triaje:\n > 100%% lista para el nuevo dia.",
    "[URGENCIAS 01:05] 'No abre historia':\n > Tenia otra historia bloqueada.",
    "[PLANTA 1 01:40] 'No va el teclado':\n > Conector PS/2 cambiado a USB.",
    "[CONSULTAS 02:15] 'Se cerro todo':\n > Clic en 'Mostrar Escritorio'.",
    "[QUIROFANO 02:40] Pantalla quirofano:\n > Resolucion ajustada a 1080p.",
    "[SOPORTE 03:00] Pregunta tipica:\n > '\u00bfEl Wi-Fi gasta bateria?'",
    "[FARMACIA 03:25] Escaner cenital:\n > Luz LED encendida y enfocada.",
    "[CPD SOTANO 1] Patch panel fibra:\n > Conectores LC bien clickeados.",
    "[UCI 03:50] Telemetria cardiaca:\n > Senal al 100%% de cobertura.",
    "[RADIOLOGIA 04:15] Servidor PACS:\n > Espacio en disco: 40 TB OK.",
    "[PLANTA 3 04:40] Pantalla deslumbra:\n > Modo oscuro activado en HIS.",
    "[TRIAJE 05:05] Teclado silicona:\n > Desinfectado y funcionando.",
    "[SISTEMAS 05:25] DNS corporativo:\n > Resolviendo 10.000 pet/s.",
    "[URGENCIAS 05:45] La impresora pita:\n > Tapa de salida mal cerrada.",
    "[PLANTA 2 06:05] 'No encuentro Word':\n > Anclado a barra de tareas.",
    "[LABORATORIO 06:20] Analizador:\n > Cable de red blindado OK.",
    "[DIRECCION 06:35] Tablet gerencia:\n > Modo avion desactivado.",
    "[QUIROFANO 06:45] Reloj quirofano:\n > Sincronizado por NTP con CPD.",
    "[ARCHIVOS 06:52] Caja de folios:\n > Apartada de la salida del rack.",
    "[CPD SOTANO 1] Guardia nocturna:\n > Equipo TIC siempre vigilante.",
    "[URGENCIAS 06:58] Fin de guardia:\n > Sobrevivimos una noche mas.",
    "[SISTEMAS 07:00] Turno entrante:\n > Informaticos de manana al relevo.",
    "[CPD SOTANO 1] Ultimo cafe:\n > Mision cumplida en Sant Joan.",
    "[HOSPITAL 07:02] Todo en calma:\n > Servidores en pie. Buen trabajo.",
};

static void trigger_auto_incident(Game *g) {
    // 20% de probabilidad: Mencion del informatico de guardia de hoy
    if ((rand() % 5) == 0) {
        int n_templates = sizeof(GUARDIA_TICKER_TEMPLATES) / sizeof(GUARDIA_TICKER_TEMPLATES[0]);
        const char *templ = GUARDIA_TICKER_TEMPLATES[rand() % n_templates];
        char buf[128];
        snprintf(buf, sizeof(buf), templ, game_get_current_guardia(g));
        game_add_ticker(g, buf);
    } else {
        // 80% de probabilidad: Incidencias hospitalarias generales
        int n_gen = sizeof(GENERAL_TICKER_POOL) / sizeof(GENERAL_TICKER_POOL[0]);
        game_add_ticker(g, GENERAL_TICKER_POOL[rand() % n_gen]);
    }
}

const char *game_get_department_name(int level) {
    int idx = level - 1;
    if (idx < 0) idx = 0;
    int max_idx = (sizeof(HOSPITAL_DEPARTMENTS) / sizeof(HOSPITAL_DEPARTMENTS[0])) - 1;
    if (idx > max_idx) idx = max_idx;
    return HOSPITAL_DEPARTMENTS[idx];
}

static bool check_collision(const Game *g, PieceType piece, int px, int py, int rot) {
    if (piece == PIECE_NONE) return false;
    int size = PIECE_SIZE[piece];
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            if (TETROMINOES[piece][rot][r][c]) {
                int bx = px + c;
                int by = py + r;
                if (bx < 0 || bx >= BOARD_WIDTH || by >= TOTAL_ROWS) {
                    return true;
                }
                if (by >= 0 && g->board[by][bx] != PIECE_NONE) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void shuffle_bag(Game *g) {
    for (int i = 0; i < 7; i++) {
        g->bag[i] = i + 1;
    }
    for (int i = 6; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = g->bag[i];
        g->bag[i] = g->bag[j];
        g->bag[j] = tmp;
    }
    g->bag_idx = 0;
}

static PieceType get_next_piece_from_bag(Game *g) {
    if (g->bag_idx >= 7) {
        shuffle_bag(g);
    }
    return (PieceType)g->bag[g->bag_idx++];
}

static void refill_next_queue(Game *g) {
    for (int i = 0; i < 5; i++) {
        if (g->next_queue[i] == PIECE_NONE) {
            g->next_queue[i] = get_next_piece_from_bag(g);
        }
    }
}

static PieceType pop_next_queue(Game *g) {
    PieceType p = g->next_queue[0];
    for (int i = 0; i < 4; i++) {
        g->next_queue[i] = g->next_queue[i + 1];
    }
    g->next_queue[4] = get_next_piece_from_bag(g);
    return p;
}

static void spawn_piece(Game *g) {
    g->current_piece = pop_next_queue(g);
    g->piece_rot = 0;
    int size = PIECE_SIZE[g->current_piece];
    g->piece_x = (BOARD_WIDTH - size) / 2;
    g->piece_y = BUFFER_HEIGHT - 2; // Spawn in buffer zone

    g->is_locking = false;
    g->lock_timer = 0;
    g->lock_resets = 0;
    g->can_hold = true;

    // Check if spawn immediately collides -> GAME OVER
    if (check_collision(g, g->current_piece, g->piece_x, g->piece_y, g->piece_rot)) {
        g->state = STATE_GAMEOVER;
        audio_play_sfx(SFX_GAMEOVER);
        game_add_ticker(g, "[CAIDA TOTAL] Servidores caidos\n > Sacan papel y bolis BIC.");
        game_set_toast(g, "¡CAIDA DEL SISTEMA!", 255, 40, 50);
        if (g->score > g->highscore) {
            g->highscore = g->score;
            game_save_highscore(g);
        }
    }
}

void game_init(Game *g) {
    memset(g, 0, sizeof(Game));
    srand((unsigned int)time(NULL));
    game_load_highscore(g);

    g->state = STATE_TITLE;
    g->menu_selected = 0;
    g->ecg_bpm = 75.0f;
    g->cpu_load = 32.0f;
    g->net_traffic = 10.0f;

    // Initial ticker messages (Formatted to fit 2 readable lines)
    game_add_ticker(g, "[INICIALIZANDO] Turno de guardia\n > Sistemas y cafetera listos.");
    game_add_ticker(g, "[CAFETERA CPD] Cafeina al 100%\n > Servidores monitorizados.");
    game_add_ticker(g, "[URGENCIAS 00:00] Silencio\n > Calma sospechosa en pasillos.");
}

void game_reset(Game *g) {
    uint32_t hs = g->highscore;
    memset(g->board, 0, sizeof(g->board));
    memset(g->particles, 0, sizeof(g->particles));
    memset(&g->toast, 0, sizeof(g->toast));

    g->highscore = hs;
    g->score = 0;
    g->lines = 0;
    g->level = 1;
    g->combo = -1;
    g->back_to_back = false;
    g->hold_piece = PIECE_NONE;
    g->can_hold = true;
    g->clearing_lines = false;
    g->screen_shake = 0.0f;

    g->state = STATE_PLAY;
    g->shift_start_time = SDL_GetTicks();
    g->last_fall_time = SDL_GetTicks();
    g->last_ticker_time = SDL_GetTicks();

    shuffle_bag(g);
    for (int i = 0; i < 5; i++) {
        g->next_queue[i] = PIECE_NONE;
    }
    refill_next_queue(g);
    spawn_piece(g);

    g->pieces_dropped = 0;
    audio_set_level_tempo(g->level);
    char init_buf[128];
    snprintf(init_buf, sizeof(init_buf), "[TURNO INICIADO] Guardia de hoy:\n > %s al frente del CPD.", game_get_current_guardia(g));
    game_add_ticker(g, init_buf);
    char init_toast[64];
    snprintf(init_toast, sizeof(init_toast), "GUARDIA: %s", game_get_current_guardia(g));
    game_set_toast(g, init_toast, 0, 230, 255);
}

void game_load_highscore(Game *g) {
    g->highscore = 0;
    FILE *f = fopen("highscore.txt", "r");
    if (f) {
        if (fscanf(f, "%u", &g->highscore) != 1) {
            g->highscore = 0;
        }
        fclose(f);
    }
}

void game_save_highscore(const Game *g) {
    FILE *f = fopen("highscore.txt", "w");
    if (f) {
        fprintf(f, "%u\n", g->highscore);
        fclose(f);
    }
}

void game_add_ticker(Game *g, const char *msg) {
    if (!msg) return;
    // Shift existing messages down
    if (g->ticker_count < MAX_TICKER_MSGS) {
        strncpy(g->ticker_msgs[g->ticker_count], msg, TICKER_MSG_LEN - 1);
        g->ticker_msgs[g->ticker_count][TICKER_MSG_LEN - 1] = '\0';
        g->ticker_count++;
    } else {
        for (int i = 0; i < MAX_TICKER_MSGS - 1; i++) {
            strncpy(g->ticker_msgs[i], g->ticker_msgs[i + 1], TICKER_MSG_LEN);
        }
        strncpy(g->ticker_msgs[MAX_TICKER_MSGS - 1], msg, TICKER_MSG_LEN - 1);
        g->ticker_msgs[MAX_TICKER_MSGS - 1][TICKER_MSG_LEN - 1] = '\0';
    }
}

void game_set_toast(Game *g, const char *text, uint8_t r, uint8_t g_col, uint8_t b) {
    if (!text) return;
    snprintf(g->toast.text, sizeof(g->toast.text), "%s", text);
    g->toast.alpha = 1.0f;
    g->toast.scale = 2.0f;
    g->toast.r = r;
    g->toast.g = g_col;
    g->toast.b = b;
}

void game_spawn_particles(Game *g, float x, float y, int count, uint8_t r, uint8_t g_col, uint8_t b) {
    for (int i = 0; i < count; i++) {
        // Find inactive particle
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (g->particles[p].life <= 0.0f) {
                float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
                float speed = 50.0f + ((float)rand() / (float)RAND_MAX) * 150.0f;
                g->particles[p].x = x;
                g->particles[p].y = y;
                g->particles[p].vx = cosf(angle) * speed;
                g->particles[p].vy = sinf(angle) * speed;
                g->particles[p].life = 1.0f;
                g->particles[p].decay = 1.5f + ((float)rand() / (float)RAND_MAX) * 2.0f;
                g->particles[p].r = r;
                g->particles[p].g = g_col;
                g->particles[p].b = b;
                g->particles[p].a = 255;
                g->particles[p].size = 2.0f + ((float)rand() / (float)RAND_MAX) * 3.0f;
                break;
            }
        }
    }
}

int game_get_ghost_y(const Game *g) {
    if (g->current_piece == PIECE_NONE) return g->piece_y;
    int gy = g->piece_y;
    while (!check_collision(g, g->current_piece, g->piece_x, gy + 1, g->piece_rot)) {
        gy++;
    }
    return gy;
}

bool game_move(Game *g, int dx, int dy) {
    if (g->state != STATE_PLAY || g->clearing_lines) return false;

    if (!check_collision(g, g->current_piece, g->piece_x + dx, g->piece_y + dy, g->piece_rot)) {
        g->piece_x += dx;
        g->piece_y += dy;

        if (dx != 0) {
            audio_play_sfx(SFX_MOVE);
        }
        if (dy > 0) {
            g->score += 1; // Soft drop point
            audio_play_sfx(SFX_SOFT_DROP);
        }

        // Reset lock timer if piece moved while touching ground
        if (check_collision(g, g->current_piece, g->piece_x, g->piece_y + 1, g->piece_rot)) {
            if (g->lock_resets < 15) {
                g->lock_timer = SDL_GetTicks();
                g->lock_resets++;
            }
        } else {
            g->is_locking = false;
        }
        return true;
    }
    return false;
}

void game_rotate(Game *g, int dir) {
    if (g->state != STATE_PLAY || g->clearing_lines || g->current_piece == PIECE_NONE) return;
    if (g->current_piece == PIECE_O) return; // O does not rotate

    int old_rot = g->piece_rot;
    int new_rot = (old_rot + dir + 4) % 4;

    // Test index for SRS wall kick table
    int kick_idx = 0;
    if (dir == 1) { // CW
        kick_idx = old_rot * 2;
    } else { // CCW
        kick_idx = (old_rot * 2 + 7) % 8;
    }

    const int (*kick_table)[5][2] = (g->current_piece == PIECE_I) ? KICKS_I : KICKS_JLSTZ;

    for (int t = 0; t < 5; t++) {
        int ox = kick_table[kick_idx][t][0];
        int oy = -kick_table[kick_idx][t][1]; // Invert Y for screen coordinates

        if (!check_collision(g, g->current_piece, g->piece_x + ox, g->piece_y + oy, new_rot)) {
            g->piece_x += ox;
            g->piece_y += oy;
            g->piece_rot = new_rot;
            audio_play_sfx(SFX_ROTATE);

            if (check_collision(g, g->current_piece, g->piece_x, g->piece_y + 1, g->piece_rot)) {
                if (g->lock_resets < 15) {
                    g->lock_timer = SDL_GetTicks();
                    g->lock_resets++;
                }
            } else {
                g->is_locking = false;
            }
            return;
        }
    }
}

void game_hard_drop(Game *g) {
    if (g->state != STATE_PLAY || g->clearing_lines || g->current_piece == PIECE_NONE) return;

    int drop_dist = 0;
    while (!check_collision(g, g->current_piece, g->piece_x, g->piece_y + 1, g->piece_rot)) {
        g->piece_y++;
        drop_dist++;
    }
    g->score += drop_dist * 2; // Hard drop score
    audio_play_sfx(SFX_HARD_DROP);
    g->screen_shake = 0.0f; // Sin vibracion al caer

    // Force immediate lock
    g->is_locking = true;
    g->lock_timer = 0; // Trigger lock now
}

void game_hold(Game *g) {
    if (g->state != STATE_PLAY || g->clearing_lines || !g->can_hold || g->current_piece == PIECE_NONE) return;

    audio_play_sfx(SFX_HOLD);
    PieceType current = g->current_piece;

    if (g->hold_piece == PIECE_NONE) {
        g->hold_piece = current;
        spawn_piece(g);
    } else {
        PieceType tmp = g->hold_piece;
        g->hold_piece = current;
        g->current_piece = tmp;
        g->piece_rot = 0;
        int size = PIECE_SIZE[g->current_piece];
        g->piece_x = (BOARD_WIDTH - size) / 2;
        g->piece_y = BUFFER_HEIGHT - 2;
        g->is_locking = false;
        g->lock_timer = 0;
        g->lock_resets = 0;
    }
    g->can_hold = false;
}

static void lock_current_piece(Game *g) {
    if (g->current_piece == PIECE_NONE) return;
    int size = PIECE_SIZE[g->current_piece];

    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            if (TETROMINOES[g->current_piece][g->piece_rot][r][c]) {
                int bx = g->piece_x + c;
                int by = g->piece_y + r;
                if (by >= 0 && by < TOTAL_ROWS && bx >= 0 && bx < BOARD_WIDTH) {
                    g->board[by][bx] = g->current_piece;
                }
            }
        }
    }

    // Check for completed lines
    g->num_clearing_rows = 0;
    for (int r = BUFFER_HEIGHT; r < TOTAL_ROWS; r++) {
        bool full = true;
        for (int c = 0; c < BOARD_WIDTH; c++) {
            if (g->board[r][c] == PIECE_NONE) {
                full = false;
                break;
            }
        }
        if (full) {
            g->clearing_rows[g->num_clearing_rows++] = r;
        }
    }

    // Track pieces dropped and trigger guardia change every 25 pieces
    g->pieces_dropped++;
    if (g->pieces_dropped % 25 == 0) {
        int new_idx = (g->pieces_dropped / 25) % 4;
        const char *new_guard = game_get_guardia_name(new_idx);
        char toast_buf[64];
        snprintf(toast_buf, sizeof(toast_buf), "¡RELEVO: %s!", new_guard);
        game_set_toast(g, toast_buf, 0, 255, 200);

        char tick_buf[128];
        snprintf(tick_buf, sizeof(tick_buf), "[CAMBIO DE GUARDIA] 25 piezas:\n > Entra al turno: %s.", new_guard);
        game_add_ticker(g, tick_buf);
    }

    if (g->num_clearing_rows > 0) {
        g->clearing_lines = true;
        g->clear_anim_start = SDL_GetTicks();
        g->screen_shake = 0.0f; // Sin vibracion
        if (g->num_clearing_rows == 4) {
            audio_play_sfx(SFX_TETRIS);
        } else {
            audio_play_sfx(SFX_LINE_CLEAR);
        }
    } else {
        g->combo = -1; // Reset combo if no line cleared
        spawn_piece(g);
    }
}

static void finish_clearing_lines(Game *g) {
    int cleared = g->num_clearing_rows;
    if (cleared == 0) return;

    for (int i = 0; i < cleared; i++) {
        int row_to_remove = g->clearing_rows[i];
        for (int r = row_to_remove; r > 0; r--) {
            for (int c = 0; c < BOARD_WIDTH; c++) {
                g->board[r][c] = g->board[r - 1][c];
            }
        }
        for (int c = 0; c < BOARD_WIDTH; c++) {
            g->board[0][c] = PIECE_NONE;
        }
    }

    g->lines += cleared;
    g->combo++;

    // Calculate score
    uint32_t line_pts = 0;
    (void)cleared;

    if (cleared == 1) {
        line_pts = 100 * g->level;
        game_set_toast(g, "¡CABLE ENCHUFADO!", 0, 240, 240);
        game_add_ticker(g, "[URGENCIAS] Ticket cerrado:\n > El monitor funciona si se enchufa.");
        g->back_to_back = false;
    } else if (cleared == 2) {
        line_pts = 300 * g->level;
        game_set_toast(g, "¡REINICIO MILAGROSO!", 40, 220, 70);
        game_add_ticker(g, "[MAGIA TIC] Apagar y encender:\n > Salvo el turno una vez mas.");
        g->back_to_back = false;
    } else if (cleared == 3) {
        line_pts = 500 * g->level;
        game_set_toast(g, "¡¡TRIPLE CAFE CON LECHE!!", 255, 170, 0);
        game_add_ticker(g, "[CPD SANT JOAN] Despeje triple:\n > 3 marrones esquivados del tiron.");
        g->back_to_back = false;
    } else if (cleared == 4) {
        if (g->back_to_back) {
            line_pts = 1200 * g->level;
            game_set_toast(g, "¡¡¡HEROE DEL CPD!!!", 255, 215, 0);
            game_add_ticker(g, "[LEYENDA] Doble Tetris seguido:\n > El turno manana debe el desayuno.");
        } else {
            line_pts = 800 * g->level;
            game_set_toast(g, "¡¡¡TETRIS DE GUARDIA!!!", 170, 45, 240);
            game_add_ticker(g, "[HEROE CPD] ¡Tetris nocturno!:\n > Panico evitado en Sant Joan.");
            g->back_to_back = true;
        }
    }

    // Combo points
    if (g->combo > 0) {
        line_pts += 50 * g->combo * g->level;
    }
    g->score += line_pts;

    if (g->score > g->highscore) {
        g->highscore = g->score;
    }

    // Level progression
    int new_level = (g->lines / 10) + 1;
    if (new_level > g->level) {
        g->level = new_level;
        audio_play_sfx(SFX_LEVELUP);
        audio_set_level_tempo(g->level);
        char toast_buf[64];
        snprintf(toast_buf, sizeof(toast_buf), "¡NIVEL %d: %s!", g->level, game_get_department_name(g->level));
        game_set_toast(g, toast_buf, 0, 255, 200);
        char tick_buf[128];
        snprintf(tick_buf, sizeof(tick_buf), "[ASCENSO] Traslado de guardia:\n > Nuevo sector: %s.", game_get_department_name(g->level));
        game_add_ticker(g, tick_buf);
    }

    g->clearing_lines = false;
    g->num_clearing_rows = 0;
    spawn_piece(g);
}

void game_update(Game *g, uint32_t delta_ms) {
    uint32_t now = SDL_GetTicks();

    // Telemetry updates
    g->ecg_phase += (float)delta_ms * 0.003f;
    if (g->ecg_phase > 2000.0f) g->ecg_phase -= 2000.0f;
    g->ecg_bpm = 72.0f + 6.0f * sinf(g->ecg_phase * 0.4f) + (float)g->level * 1.5f;
    g->cpu_load = 28.0f + 15.0f * sinf(g->ecg_phase * 0.8f) + (float)g->level * 2.5f;
    if (g->cpu_load > 98.0f) g->cpu_load = 98.0f;

    // Toast banner fade
    if (g->toast.alpha > 0.0f) {
        g->toast.alpha -= (float)delta_ms * 0.0012f;
        if (g->toast.alpha < 0.0f) g->toast.alpha = 0.0f;
    }

    // Screen shake decay
    if (g->screen_shake > 0.0f) {
        g->screen_shake -= (float)delta_ms * 0.02f;
        if (g->screen_shake < 0.0f) g->screen_shake = 0.0f;
    }

    // Particles update
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g->particles[i].life > 0.0f) {
            float dt = (float)delta_ms / 1000.0f;
            g->particles[i].x += g->particles[i].vx * dt;
            g->particles[i].y += g->particles[i].vy * dt;
            g->particles[i].vy += 180.0f * dt; // Gravity
            g->particles[i].life -= g->particles[i].decay * dt;
            if (g->particles[i].life < 0.0f) g->particles[i].life = 0.0f;
        }
    }

    // Periodic auto ticker message
    if (now - g->last_ticker_time > 7500) {
        g->last_ticker_time = now;
        trigger_auto_incident(g);
    }

    if (g->state != STATE_PLAY) return;

    g->play_duration_sec = (now - g->shift_start_time) / 1000;

    // Line clear animation in progress
    if (g->clearing_lines) {
        if (now - g->clear_anim_start >= 180) {
            finish_clearing_lines(g);
        }
        return;
    }

    // Delayed Auto Shift (DAS) for left/right keys
    const uint32_t DAS_DELAY = 160;
    const uint32_t ARR_INTERVAL = 35;
    if (g->key_left_held) {
        if (now - g->key_left_timer >= DAS_DELAY) {
            game_move(g, -1, 0);
            g->key_left_timer = now - (DAS_DELAY - ARR_INTERVAL);
        }
    }
    if (g->key_right_held) {
        if (now - g->key_right_timer >= DAS_DELAY) {
            game_move(g, 1, 0);
            g->key_right_timer = now - (DAS_DELAY - ARR_INTERVAL);
        }
    }
    if (g->key_down_held) {
        if (now - g->key_down_timer >= 45) {
            game_move(g, 0, 1);
            g->key_down_timer = now;
        }
    }

    // Natural gravity drop calculation
    // Level 1: ~900ms, Level 10: ~140ms
    float speed_ms = 900.0f * powf(0.82f, (float)(g->level - 1));
    if (speed_ms < 90.0f) speed_ms = 90.0f;

    if (now - g->last_fall_time >= (uint32_t)speed_ms) {
        g->last_fall_time = now;
        if (!game_move(g, 0, 1)) {
            // Can't move down, start or continue lock delay
            if (!g->is_locking) {
                g->is_locking = true;
                g->lock_timer = now;
            }
        }
    }

    // Check lock delay expiration (500ms)
    if (g->is_locking && check_collision(g, g->current_piece, g->piece_x, g->piece_y + 1, g->piece_rot)) {
        if (now - g->lock_timer >= 500) {
            lock_current_piece(g);
        }
    }
}

void game_handle_key_down(Game *g, int keycode) {
    uint32_t now = SDL_GetTicks();

    // Global volume / music controls
    if (keycode == SDLK_m) {
        audio_toggle_music();
        if (audio_is_music_enabled()) {
            game_set_toast(g, "MÚSICA: ACTIVADA", 0, 255, 128);
        } else {
            game_set_toast(g, "MÚSICA: SILENCIADA", 255, 100, 100);
        }
        return;
    }
    if (keycode == SDLK_t) {
        audio_next_track();
        char buf[64];
        snprintf(buf, sizeof(buf), "PISTA: %s", audio_get_track_name());
        game_set_toast(g, buf, 0, 230, 255);
        return;
    }
    if (keycode == SDLK_PLUS || keycode == SDLK_KP_PLUS || keycode == SDLK_EQUALS) {
        audio_volume_up();
        char buf[32];
        snprintf(buf, sizeof(buf), "VOLUMEN: %d%%", (int)(audio_get_volume() * 100.0f));
        game_set_toast(g, buf, 100, 220, 255);
        return;
    }
    if (keycode == SDLK_MINUS || keycode == SDLK_KP_MINUS) {
        audio_volume_down();
        char buf[32];
        snprintf(buf, sizeof(buf), "VOLUMEN: %d%%", (int)(audio_get_volume() * 100.0f));
        game_set_toast(g, buf, 100, 220, 255);
        return;
    }

    // Help / About toggle
    if (keycode == SDLK_F1 || keycode == SDLK_h) {
        if (g->state == STATE_ABOUT) {
            g->state = g->prev_state;
        } else {
            g->prev_state = g->state;
            g->state = STATE_ABOUT;
        }
        return;
    }

    // Title state controls
    if (g->state == STATE_TITLE) {
        if (keycode == SDLK_UP || keycode == SDLK_w) {
            g->menu_selected = (g->menu_selected + 2) % 3;
            audio_play_sfx(SFX_MOVE);
        } else if (keycode == SDLK_DOWN || keycode == SDLK_s) {
            g->menu_selected = (g->menu_selected + 1) % 3;
            audio_play_sfx(SFX_MOVE);
        } else if (keycode == SDLK_RETURN || keycode == SDLK_SPACE || keycode == SDLK_KP_ENTER) {
            if (g->menu_selected == 0) {
                game_reset(g);
            } else if (g->menu_selected == 1) {
                g->prev_state = STATE_TITLE;
                g->state = STATE_ABOUT;
            } else if (g->menu_selected == 2) {
                SDL_Event quit_ev;
                quit_ev.type = SDL_QUIT;
                SDL_PushEvent(&quit_ev);
            }
        }
        return;
    }

    // Game over controls
    if (g->state == STATE_GAMEOVER) {
        if (keycode == SDLK_r || keycode == SDLK_RETURN || keycode == SDLK_SPACE) {
            game_reset(g);
        } else if (keycode == SDLK_ESCAPE) {
            g->state = STATE_TITLE;
        }
        return;
    }

    // Pause controls
    if (g->state == STATE_PAUSE) {
        if (keycode == SDLK_p || keycode == SDLK_ESCAPE) {
            g->state = STATE_PLAY;
            g->last_fall_time = now;
        } else if (keycode == SDLK_r) {
            game_reset(g);
        } else if (keycode == SDLK_q) {
            g->state = STATE_TITLE;
        }
        return;
    }

    // Active gameplay controls
    if (g->state == STATE_PLAY) {
        if (keycode == SDLK_p || keycode == SDLK_ESCAPE) {
            g->state = STATE_PAUSE;
            return;
        }
        if (keycode == SDLK_r) {
            game_reset(g);
            return;
        }

        if (keycode == SDLK_LEFT || keycode == SDLK_a) {
            g->key_left_held = true;
            g->key_left_timer = now;
            game_move(g, -1, 0);
        } else if (keycode == SDLK_RIGHT || keycode == SDLK_d) {
            g->key_right_held = true;
            g->key_right_timer = now;
            game_move(g, 1, 0);
        } else if (keycode == SDLK_DOWN || keycode == SDLK_s) {
            g->key_down_held = true;
            g->key_down_timer = now;
            game_move(g, 0, 1);
        } else if (keycode == SDLK_UP || keycode == SDLK_w || keycode == SDLK_x) {
            game_rotate(g, 1); // Clockwise
        } else if (keycode == SDLK_z || keycode == SDLK_q) {
            game_rotate(g, -1); // Counter-clockwise
        } else if (keycode == SDLK_SPACE) {
            game_hard_drop(g);
        } else if (keycode == SDLK_c || keycode == SDLK_LSHIFT || keycode == SDLK_RSHIFT) {
            game_hold(g);
        }
    }
}

void game_handle_key_up(Game *g, int keycode) {
    if (keycode == SDLK_LEFT || keycode == SDLK_a) {
        g->key_left_held = false;
    } else if (keycode == SDLK_RIGHT || keycode == SDLK_d) {
        g->key_right_held = false;
    } else if (keycode == SDLK_DOWN || keycode == SDLK_s) {
        g->key_down_held = false;
    }
}

int game_get_piece_cell(PieceType piece, int rot, int r, int c) {
    if (piece < 1 || piece > 7) return 0;
    if (rot < 0 || rot > 3) return 0;
    if (r < 0 || r >= 4 || c < 0 || c >= 4) return 0;
    return TETROMINOES[piece][rot][r][c];
}

int game_get_piece_size(PieceType piece) {
    if (piece < 1 || piece > 7) return 0;
    return PIECE_SIZE[piece];
}
