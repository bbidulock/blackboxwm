$set 1 #BaseDisplay

1 %s:  Un error de X: %s(%d) clave de operación %d/%d\n  recurso 0x%lx\n
2 %s: señal %d capturada\n
3 cerrando\n
4 cancelando... generando 'core'\n
5 BaseDisplay::BaseDisplay: la conexión al servidor de X falló.\n
6 BaseDisplay::BaseDisplay: no se pude marcar la conexión de despliegue como close-on-exec\n
7 BaseDisplay::eventLoop(): quitando la ventana errónea de la cola de eventos\n

$set 2 #Basemenu

1 Menú de Blackbox

$set 3 #Configmenu

1 Opciones de configuración
2 Modelo de enfoque
3 Ubicación de ventanas
4 Simulación de colores en imágenes
5 Movimiento opaco de ventanas
6 Maximización total
7 Enfocar ventanas nuevas
8 Enfocar la última ventana en el escritorio
9 Seleccionar para enfocar
10 Enfocar sin forma estricta
11 Elevar automáticamente las ventanas
12 Ubicación inteligente (filas)
13 Ubicación inteligente (columnas)
14 Ubicación en cascada
15 De izquierda a derecha
16 De derecha a izquierda
17 De arriba hacia abajo
18 De abajo hacia arriba

$set 4 #Icon

1 Iconos
2 Sin nombre

$set 5 #Image

1 BImage::render_solid: error creando el pixmap\n
2 BImage::renderXImage: error creando el XImage\n
3 BImage::renderXImage: visual no soportado\n
4 BImage::renderPixmap: error creando el pixmap\n
5 BImageControl::BImageControl: mapa no válido de colores tamaño %d (%d/%d/%d) - reduciendo\n
6 BImageControl::BImageControl: error en la asignación del mapa de colores\n
7 BImageControl::BImageControl: no se puede asignar el color %d/%d/%d\n
8 BImageControl::~BImageControl: reserva de pixmaps - liberando %d pixmaps\n
9 BImageControl::renderImage: reserva de pixmaps grande, forzando limpieza\n
10 BImageControl::getColor: error evaluando el color: '%s'\n
11 BImageControl::getColor: error asignando el color: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: un error ocurrió mientras cuestionaba el servidor X.\n  otro gestor de ventanas se está ejecutando en conexión de despliegue %s.\n
2 BScreen::BScreen: gestionando la pantalla %d usando visual 0x%lx, profundidad %d\n
3 BScreen::LoadStyle(): no se puede cargar el tipo de letra '%s'\n
4 BScreen::LoadStyle(): no se puede cargar el tipo de letra por omisión.\n
5 %s: archivo de menú vacío\n
6 xterm
7 Reiniciar
8 Salir
9 BScreen::parseMenuFile: [exec] error, no hay etiqueta de menú ni comando definido\n
10 BScreen::parseMenuFile: [exit] error, no hay etiqueta de menú definida\n
11 BScreen::parseMenuFile: [style] error, no hay etiqueta de menú ni archivo \
definido\n
12 BScreen::parseMenuFile: [config] error, no hay etiqueta de menú definida\n
13 BScreen::parseMenuFile: [include] error, no hay nombre de archivo definido\n
14 BScreen::parseMenuFile: [include] error, '%s' no es un archivo regular\n
15 BScreen::parseMenuFile: [submenu] error, no hay etiqueta de menú definida\n
16 BScreen::parseMenuFile: [restart] error, no hay etiqueta de menú definida\n
17 BScreen::parseMenuFile: [reconfig] error, no hay etiqueta de menú definida\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, no hay directorio definido\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, '%s' no es un directorio\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, '%s' no existe\n
21 BScreen::parseMenuFile: [workspaces] error, no hay etiqueta de menú definida\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d

$set 7 #Slit

1 Slit
2 Dirección de slit
3 Ubicación de slit

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4 %02d:%02d
5 %02d:%02d %sm
6 p
7 a
8 Escritorio
9 Cambiar el nombre del escritorio actual
10 Ubicación de la barra de herramientas

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: creando 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres falló\n
3 BlackboxWindow::BlackboxWindow: no se puede encontrar la pantalla para la ventana \
  raíz 0x%lx\n
4 Sin nombre
5 BlackboxWindow::mapRequestEvent() para 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() para 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: el padre nuevo para 0x%lx es la raíz\n

$set 10 #Windowmenu

1 Enviar a ...
2 Enrollar
3 Convertir en icono
4 Maximizar
5 Elevar
6 Bajar
7 Fijar
8 Matar al cliente
9 Cerrar

$set 11 #Workspace

1 Escritorio %d

$set 12 #Workspacemenu

1 Escritorios
2 Escritorio nuevo
3 Quitar el último escritorio

$set 13 #blackbox

1 Blackbox::Blackbox: no encontré pantallas manejables, cerrando\n
2 Blackbox::process_event: MapRequest para 0x%lx\n

$set 14 #Common

1 Sí
2 No

3 Dirección
4 Horizontal
5 Vertical

6 Siempre encima/arriba

7 Ubicación 
8 Arriba a la izquierda
9 Centro a la izquierda
10 Abajo a la izquierda
11 Arriba al centro
12 Abajo al centro
13 Arriba a la derecha
14 Centro a la derecha
15 Abajo a la derecha

$set 15 #main

1 error: '-rc' requiere un argumento
2 error: '-display' requiere un argumento
3 cuidado: no se puede establecer la variable de ambiente 'DISPLAY'
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <string> conexión de despliegue.\n\
  -rc <string>      archivo alternativo de recuros.\n\
  -version          mostrar la versión y cerrar.\n\
  -help             mostrar este texto de ayuda y cerrar.\n\n
5 Opciones durante la compilación:\n\
  Información extra para depuración:               %s\n\
  Entrelazado:                                     %s\n\
  Forma:                                           %s\n\
  Slit:                                            %s\n\
  8bpp simulación ordenada de colores en imágenes: %s\n\n

$set 16 #bsetroot

1 %s: error: es necesario especifiar uno de: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        conexión de despliegue\n\
  -mod <x> <y>             diseño de modula \n\
  -foreground, -fg <color> color del primer plano de modula\n\
  -background, -bg <color> color del fondo de modula\n\n\
  -gradient <texture>      textura de la pendiente\n\
  -from <color>            color inicial de la pendiente\n\
  -to <color>              color final de la pendiente\n\n\
  -solid <color>           color sólido\n\n\
  -help                    mostrar este texto de ayuda y salir\n

