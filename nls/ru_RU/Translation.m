$set 1 #BaseDisplay

1 %s:  ошибка X сервера: %s(%d) значения %d/%d\n ресурс 0x%lx\n
2 %s: получен сигнал %d \n
3 отключение\n
4 Отмена... создается дамп core\n
5 BaseDisplay::BaseDisplay: соединение с X сервером провалено.\n
6 BaseDisplay::BaseDisplay: нет возможности пометить активный дисплей как "закрытый-на-исполнение"\n
7 BaseDisplay::eventLoop(): плохое окно удаляется из очереди событий\n

$set 2 #Basemenu

1 Меню BlackBox

$set 3 #Configmenu

1 Конфигурация
2 Модель фокусировки
3 Положение окна
4 Image Dithering
5 Перемещение заполненных окон
6 Полная максимизация
7 Перемещать фокус на новые окна
8 Фокус на последнее окно рабочего стола
9 Фокус по щелчку
10 Фокус по перемещению
11 Автовсплытие
12 "Умное" расположение (по горизонтали)
13 "Умное" расположение (по вертикали)
14 Каскадом
15 Слева направо
16 Справа налево
17 Сверху вниз
18 Снизу вверх

$set 4 #Icon

1 Свернутые\nокна
2 Безымянные

$set 5 #Image

1 BImage::render_solid: ошибка создания pixmap\n
2 BImage::renderXImage: ошибка создания XImage\n
3 BImage::renderXImage: неподдерживаемый тип визуализации\n
4 BImage::renderPixmap: ошибка создания pixmap\n
5 BImageControl::BImageControl: неверный размер таблицы цветов %d (%d/%d/%d) - удаляется\n
6 BImageControl::BImageControl: ошибка размещения таблицы цветов\n
7 BImageControl::BImageControl: невозможно разместить цвет %d/%d/%d в памяти\n
8 BImageControl::~BImageControl: кэш точечных изображений - освобождается %d блоков\n
9 BImageControl::renderImage: переполнение кэша, производится быстрая очистка\n
10 BImageControl::getColor: ошибка разбора строки, описывающей цвет: '%s'\n
11 BImageControl::getColor: ошибка размещения цвета в памяти: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: произошла ошибка при обращении к X серверу.\n  \
другой менеджер окон уже запущен на дисплее %s.\n
2 BScreen::BScreen: обслуживается экран %d, используемый тип визуализации 0x%lx, глубина цвета %d\n
3 BScreen::LoadStyle(): нет возможности загрузить шрифт '%s'\n
4 BScreen::LoadStyle(): нет возможности загрузить предопределенный шрифт.\n
5 %s: пустой файл меню\n
6 Xterm
7 Перезапуск
8 Выход
9 BScreen::parseMenuFile: ошибка [exec], не определено название пункта меню и/или название комманды\n
10 BScreen::parseMenuFile: ошибка [exit], не определено название пункта меню\n
11 BScreen::parseMenuFile: ошибка [style], не определено название пункта меню и/или имя файла\n
12 BScreen::parseMenuFile: ошибка [config], не определено название пункта меню\n
13 BScreen::parseMenuFile: ошибка [include], не определено имя файла\n
14 BScreen::parseMenuFile: ошибка [include], '%s' не обычный файл\n
15 BScreen::parseMenuFile: ошибка [submenu], не определено название пункта меню\n
16 BScreen::parseMenuFile: ошибка [restart], не определено название пункта меню\n
17 BScreen::parseMenuFile: ошибка [reconfig], не определено название пункта меню\n
18 BScreen::parseMenuFile: ошибка [stylesdir/stylesmenu], не определено имя директории\n
19 BScreen::parseMenuFile: ошибка [stylesdir/stylesmenu], '%s' не директория\n
20 BScreen::parseMenuFile: ошибка [stylesdir/stylesmenu], '%s' не существует\n
21 BScreen::parseMenuFile: ошибка [workspaces], не определено название пункта меню\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d


$set 7 #Slit

1 Докер
2 Ориентация докера
3 Местоположения докера

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 пп
7 дп
8 Тулбар
9 Редактировать имя текущего рабочего стола
10 Местоположение тулбара

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: создается 0x%lx\n
2 BlackboxWindow::BlackboxWindow: провален процесс XGetWindowAttributres\n
3 BlackboxWindow::BlackboxWindow: нет возможности найти экран для корневого окна 0x%lx\n
4 Безымянное
5 BlackboxWindow::mapRequestEvent() для 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() для 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 Отправить на ...
2 Свернуть в заголовок
3 Свернуть в иконку
4 Максимизировать
5 Поднять наверх
6 Опустить вниз
7 Приклеить
8 Убить клиентское приложение
9 Закрыть

$set 11 #Workspace

1 Рабочий стол %d

$set 12 #Workspacemenu

1 Рабочие столы
2 Новый рабочий стол
3 Удалить последний

$set 13 #blackbox

1 Blackbox::Blackbox: не найдено экранов для обслуживания, отмена...\n
2 Blackbox::process_event: MapRequest for 0x%lx\n

$set 14 #Common

1 Да
2 Нет

3 Направление
4 По горизонтали
5 По вертикали

6 Всегда наверху

7 Местоположение
8 Слева вверху
9 Слева по центру
10 Слева внизу
11 Сверху по центру
12 Снизу по центру
13 Справа сверху
14 Справа по центру
15 Справа внизу

$set 15 #main

1 ошибка: '-rc' требует наличие аргумента\n
2 ошибка: '-display' требует наличие аргумента\n
3 предупреждение: невозможно установить переменную окружения 'DISPLAY'\n
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <string>\t\tиспользовать заданный дисплей.\n\
  -rc <string>\t\t\tиспользовать альтернативный файл ресурсов.\n\
  -version\t\t\tвывести номер версии и выйти.\n\
  -help\t\t\t\tвывести эту подсказку и выйти.\n\n
5 Compile time options:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

$set 16 #bsetroot

1 %s: ошибка: необходимо задать один из следующих ключей: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        соединение с дисплеем\n\
  -mod <x> <y>             макет клетки\n\
  -foreground, -fg <color> цвет переднего плана клетки\n\
  -background, -bg <color> цвет фона клетки\n\n\
  -gradient <texture>      градиент\n\
  -from <color>            начальный цвет градиента\n\
  -to <color>              конечный цвет градиента\n\n\
  -solid <color>           сплошной цвет\n\n\
  -help                    вывести эту подсказку и выйти\n

