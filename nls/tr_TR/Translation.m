$set 1 #BaseDisplay

1 %s : X hatasý : %s ( %d ) opkodlar %d/%d\n  kaynak 0x%lx\n
2 %s : %d sinyali alýndý \n
3 kapatýlýyorum\n
4 kapatýlýyorum ... çöküntüyü býrakýyorum\n
5 BaseDisplay::BaseDisplay : X sunucusuna baðlanýlýnamadý .\n
6 BaseDisplay::BaseDisplay : çalýþtýrýrken kapatmada ekrana baðlanýlýnamadý \n
7 BaseDisplay::eventLoop() : eylem kuyruðundaki 'kötü' pencereyi siliyorum \n

$set 2 #Basemenu

1 Blackbox mönüsü

$set 3 #Configmenu

1 Ayarlar
2 Foküsleme
3 Pencere yerleþimi
4 Resim oluþturmasý
5 Ekraný içerikli taþý
6 Tam ekranla, vallahi
7 Yeni pencereleri foküsle
8 Masaüstündeki son pencereyi foküsle
9 Týklayarak foküsle
10 Aðýr foküsle
11 Otomatikman yükselt
12 Akýllý yerleþim( Sýralar )
13 Akýllý yerleþim( Sütunlar )
14 Cascade Placement
15 Soldan saða
16 Saðdan sola
17 Üstten aþaða
18 Alttan üste

$set 4 #Icon

1 Ikonalar
2 Isimsiz

$set 5 #Image

1 BImage::render_solid : resmi yaratamadým\n
2 BImage::renderXImage : XImage'i yaratamadým\n
3 BImage::renderXImage : desteklenmeyen görünüþ( renk derinliði )\n
4 BImage::renderPixmap : resmi yaratamadým\n
5 BImageControl::BImageControl : geçersiz renk haritasý büyüklüðü %d (%d/%d/%d) - azaltýyorum\n
6 BImageControl::BImageControl : renk haritasý ayrýlanamadý\n
7 BImageControl::BImageControl : rengi ayrýrken hata oldu : %d/%d/%d\n
8 BImageControl::~BImageControl : resim arabelleði - %d resim temizlendi\n
9 BImageControl::renderImage : arabellek büyük, temizlemeye baþlýyorum\n
10 BImageControl::getColor : renk tarama hatasý : '%s'\n
11 BImageControl::getColor : renk ayýrma hatasý : '%s'\n

$set 6 #Screen

1 BScreen::BScreen : X sunucusunu sorgularken hata oldu.\n  \
%s ekranýnda baþka bir pencere yöneticisi çalýþýyor gibi.\n
2 BScreen::BScreen : %d ekraný, 0x%lx görünümüyle , %d derinliðiyle\n
3 BScreen::LoadStyle() : '%s' yazý tipi yüklenemedi.\n
4 BScreen::LoadStyle(): önayarlý yazý tipi yüklenemedi.\n
5 %s : boþ mönü dosyasý\n
6 X komutasý
7 Yeniden baþla
8 Çýk
9 BScreen::parseMenuFile : [exec] hatasý, mönü yaftasý ve/yada komuta belirlenmedi\n
10 BScreen::parseMenuFile : [exit] hatasý, mönü yaftasý belirlenmedi\n
11 BScreen::parseMenuFile : [style] hatasý, mönü yaftasý ve/yada dosya adý belirlenmedi\n
12 BScreen::parseMenuFile: [config] hatasý, mönü yaftasý belirlenmedi\n
13 BScreen::parseMenuFile: [include] hatasý, dosya adý belirlenmedi\n
14 BScreen::parseMenuFile: [include] hatasý, '%s' vasat bir dosya deðil\n
15 BScreen::parseMenuFile: [submenu] hatasý, mönü yaftasý belirlenmedi\n
16 BScreen::parseMenuFile: [restart] hatasý, mönü yaftasý belirlenmedi\n
17 BScreen::parseMenuFile: [reconfig] hatasý, mönü yaftasý belirlenmedi\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] hatasý, dizin adý belirlenmedi\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] hatasý, '%s' bir dizin \
deðildir\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] hatasý, '%s' var deðil\n
21 BScreen::parseMenuFile: [workspaces] hatasý, mönü yaftasý belirlenmedi\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 Y: %4d x E: %4d

$set 7 #Slit

1 Slit
2 Slit yönü
3 Slit yerleþimi

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Blackbox çubuðu
9 Geçerli masaüstü ismini deðiþtir
10 Blackbox çubuðunun yerleþimi

$set 9 #Window


1 BlackboxWindow::BlackboxWindow : 0x%lx'i yarat#_yorum\n
2 BlackboxWindow::BlackboxWindow : XGetWindowAttributres baþarýsýz oldu\n
3 BlackboxWindow::BlackboxWindow : 0x%lx ana penceresi için ekraný belirleyemedim\n
4 Isimsiz
5 0x%lx için BlackboxWindow::mapRequestEvent()\n
6 0x%lx için BlackboxWindow::unmapNotifyEvent()\n
7 BlackboxWindow::unmapnotifyEvent: 0x%lx'i ana pencereyi boya\n

$set 10 #Windowmenu

1 Gönder ...
2 Topla
3 Ikonalaþtýr
4 Azamileþtir
5 Alçalt
6 Yükselt
7 Yapýþýk
8 Öldür
9 Kapat

$set 11 #Workspace

1 Masaüstü %d

$set 12 #Workspacemenu

1 Masaüstleri
2 Yeni bir masaüstü
3 Son masaüstünü sil

$set 13 #blackbox

1 Blackbox::Blackbox: yönetebilinen ekran bulunamadý, bitiriliyorum\n
2 Blackbox::process_event: 0x%lx için MapRequest\n

$set 14 #Common

1 Evet
2 Hayýr

3 Yön
4 Ufki
5 Dikey

6 Her zaman üstte

7 Yerleþim
8 Sol üst
9 Sol orta 
10 Sol alt
11 Üst orta
12 Alt orta
13 Sað üst
14 Sað orta
15 Sað üst

$set 15 #main

1 hata : '-rc' bir argüman bekler\n
2 hata : '-display' bir argüman bekler\n
3 ikaz : 'DISPLAY' verisini oturtamadým\n
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <metin>\t\tekraný kullan.\n\
  -rc <metin>\t\t\tbaþka bir ayarlama dosyasýný kullan.\n\
  -version\t\t\tnesil bilgisini gösterir ve çýkar.\n\
  -help\t\t\t\tbu yardým iletisini gösterir ve çýkar.\n\n
5 Denetleme seçenekleri :\n\
  Bilgilendirme\t\t\t%s\n\
  Týzlama:\t\t\t%s\n\
  Gölgeleme:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  R8b'e göre týzla:\t%s\n\n

$set 16 #bsetroot

1 %s : hata : -solid, -mod yada -gradient'den birisini belirlemek zorundasýn\n
2 %s 2.0 : Tel'if hakký (c) 1997-2000 Brad Hughes\n\n\
  -display <metin>         ekran belirlemesi\n\
  -mod <x> <y>             bölüþüm iþlemi\n\
  -foreground, -fg <renk>  bölüþüm önalaný\n\
  -background, -bg <renk>  bölüþüm ardalaný\n\n\
  -gradient <kaplam>       geçiþim kaplamý\n\
  -from <renk>             geçiþim baþlama rengi\n\
  -to <renk>               geçiþim bitiþ rengi\n\n\
  -solid <renk>            tek renk\n\n\
  -help                    bu yardým iletisini göster ve çýk\n

