# HondaCL250 Telemetry — Düzeltme ve Sağlamlaştırma Raporu

> Bu doküman Claude Code ile çalışılmak üzere hazırlanmıştır.
> Görevler sıralıdır. Bir faz bitmeden sonrakine geçilmez.

---

## 0. Proje bağlamı

**Donanım:** ESP32-S3 · TWAI CAN (GPIO 4/5) · MPU6050 I2C (GPIO 1/2) · Nextion UART2 (GPIO 17/18)
**Araç:** Honda CL250, DLC/OBD-II portu, 500 kbps CAN, UDS (ISO 14229)
**Yapı:** PlatformIO · C++ · `IModule` arayüzü · süperloop · `SystemState` merkezi veri deposu

### Mevcut doğrulama durumu — kritik

| Bileşen | Durum |
|---|---|
| CAN/UDS okuma (RPM, hız, hararet, TPS, voltaj) | ✅ **Gerçek motorda doğrulandı** |
| Nextion HMI | 🔶 Yeni başlandı |
| MPU6050 / lean angle | ❌ **Donanım bağlı değil — kod hiç çalıştırılmadı** |
| BLE GATT | ❌ Doğrulanmadı |
| Wi-Fi SoftAP / HTTP API | ❌ Doğrulanmadı |
| Mobil uygulamalar (Flutter, PWA) | ❌ Doğrulanmadı |

**Sonuç:** Doğrulanmamış kodu "düzeltmek" anlamsızdır. Önce ölçüm ve doğrulama altyapısı, sonra düzeltme.

---

## 1. Claude Code için çalışma kuralları

**Uy:**
- Her görev **ayrı commit**. Commit mesajı ne değiştiğini ve nedenini yazsın. Toplu commit yapma.
- Bir görevi bitirmeden diğerine geçme.
- Değişiklikten önce ilgili dosyayı oku; varsayımla yazma.
- Mimariyi değiştiren her karar için **önce seçenekleri sun, onay bekle.**
- Basit çözümü tercih et. Karmaşıklık ancak ölçülmüş bir problemi çözüyorsa eklenir.
- Ek donanım/kütüphane gerektiren öneri getirirken maliyetini belirt.

**Yapma:**
- Çalışan CAN/UDS okuma mantığını yeniden yazma — doğrulanmış tek parça o.
- Tüm projeyi bir seferde yeniden yapılandırma.
- Mobil uygulamalara (Flutter/PWA) bu raporun dışında dokunma.
- Yeni özellik ekleme. Bu bir sağlamlaştırma turu.
- Test edilemeyen değişiklik önerme; her görevin doğrulama adımı var.

---

## FAZ 0 — Görünürlük
*Ölçmeden düzeltme yapılmaz.*

### G0.1 — Döngü zamanlaması enstrümantasyonu
> ✅ **TAMAMLANDI** (commit `0e1df9c`) — `src/main.cpp`. Build ile doğrulandı (statik); osiloskop/GPIO8 ölçümü kullanıcı tarafından yapılmalı.
**Problem:** RPM için 50 ms hedefi var ama gerçek döngü süresi ve seğirme bilinmiyor. Nextion UART, Wi-Fi ve BLE aynı döngüde.
**Yap:** Ana döngünün başında bir hata ayıklama GPIO'sunu yükselt, sonunda düşür. Ayrıca her modülün `update()` süresini `esp_timer_get_time()` ile ölçüp 10 saniyede bir seri porta min/ortalama/maks yazdır.
**Kabul:** Seri çıktıda modül başına süre ve toplam döngü süresi görünüyor.
**Doğrulama:** Osiloskop veya mantık analizörüyle GPIO darbe genişliği okunuyor; en kötü durum döngü süresi sayı olarak biliniyor.

### G0.2 — Modül sağlık ve hata sayaçları
> ✅ **TAMAMLANDI** (commit `666060a`) — `src/IModule.h`, tüm modüller, `src/main.cpp`. Build ile doğrulandı.
**Problem:** Bir modül başarısız olduğunda sistem bunu bilmiyor; `begin()` false dönse bile `update()` çağrılmaya devam ediyor.
**Yap:** `IModule`'e `bool isHealthy() const` ve hata sayacı ekle. `main.cpp`'de `begin()` başarısız olan modülü `update()` döngüsünden çıkar ve durumu logla.
**Kabul:** IMU bağlı olmadığında sistem çalışmaya devam ediyor ve seri portta "IMU unavailable" görünüyor.

### G0.3 — Veri bayatlık (staleness) takibi
> ✅ **TAMAMLANDI** (commit `4322803`) — `src/SystemState.h`, `src/HondaCANModule.cpp`, `src/IMUModule.cpp`, `src/SerialLoggerModule.cpp`. Build ile doğrulandı; CAN kablosu çekilerek fiziksel doğrulama kullanıcıda.
**Problem:** CAN kesildiğinde Nextion son değeri göstermeye devam eder. Sürücü yanlış bilgi okur — bu, bilgi göstermemekten tehlikelidir.
**Yap:** `SystemState`'teki her sinyale `last_update_ms` zaman damgası ekle. Tanımlı bir eşiği (örn. RPM için 500 ms) aşan veri "bayat" sayılsın.
**Kabul:** CAN kablosu çıkarıldığında bayatlık bayrağı set oluyor.

---

## FAZ 1 — Kritik hatalar
*Sessiz veri bozulması ve donma riskleri.*

### G1.1 — BLE paketinde endianness doğrulaması ⚠️
> ✅ **TAMAMLANDI** (commit `45f8dd2`) — `src/BLETelemetryPacket.h`, `mobile_app/app.js`, `mobile_app/flutter_app/lib/models/telemetry_data.dart`. Üç taraf zaten LE kullanıyordu, belgelendi; app.js'te 12→13 bayt uzunluk kontrolü hatası da düzeltildi. `node --check` ile app.js sözdizimi doğrulandı.
**Problem:** ESP32 little-endian; Web Bluetooth `DataView` varsayılan olarak big-endian okur. `app.js` ve Dart tarafında `littleEndian: true` geçilmemişse değerler bozuk.
**Yap:** `BLETelemetryPacket.h`, `app.js` ve Dart modelindeki bayt sırasını karşılaştır. Bayt sırasını **açıkça little-endian** olarak sabitle ve üç yerde de yorum satırıyla belgele.
**Kabul:** Bilinen sabit değerlerle (örn. RPM = 0x1234) test paketi gönderildiğinde her iki istemci de aynı değeri gösteriyor.

### G1.2 — Paylaşılan durum veri yarışı ⚠️
> ✅ **TAMAMLANDI** (commit `e27c90d`, kullanıcı onayı: **Seçenek B — FreeRTOS kuyruğu**) — `src/BLEServerModule.h/.cpp`. Build ile doğrulandı.
**Problem:** ESP32'de BLE GATT yazma geri çağrımları BLE yığınının kendi task'ında, HTTP istekleri ise Wi-Fi task'ında çalışır. Bunlar `SystemState`'e yazarken ana döngü okuyorsa korumasız eşzamanlı erişim vardır.
**Yap:** Önce hangi geri çağrımların hangi bağlamda çalıştığını tespit et ve raporla. Sonra **iki seçenek sun ve onay bekle:**
- **A)** `SystemState` erişimini mutex ile koru (basit, ama ana döngüyü bloklayabilir)
- **B)** Geri çağrımlar FreeRTOS kuyruğuna yazsın, ana döngü kuyruğu boşaltsın (daha temiz, kilitsiz)
**Kabul:** Seçilen çözüm uygulandı; telematik verisi (şarkı adı, navigasyon) yazılırken bozulma gözlenmiyor.

### G1.3 — TWAI bus-off kurtarma ⚠️
> ✅ **TAMAMLANDI** (commit `55cbafa`) — `src/HondaCANModule.h/.cpp`. Temel kurtarma zaten vardı, üstel geri çekilme + olay loglama eklendi. Build ile doğrulandı; fiziksel CAN kısa devre testi kullanıcıda.
**Problem:** Hata sayaçları dolduğunda TWAI denetleyicisi bus-off durumuna girer ve kendiliğinden geri dönmez. Cihaz sessizce ölür.
**Yap:** `twai_get_status_info()` ile periyodik durum kontrolü. Bus-off tespitinde `twai_initiate_recovery()` ile kurtarma, üstel geri çekilmeli yeniden deneme, ve olayın loglanması.
**Kabul:** CAN H/L hattı kısa devre edilip bırakıldığında sistem kendini toparlıyor ve olay logda görünüyor.

### G1.4 — Watchdog ve güvenli durum
> ✅ **TAMAMLANDI** (commit `4157f52`) — `src/main.cpp` (TWDT + reset sebebi), `src/NextionModule.h/.cpp` (stale sentinel -999). Build ile doğrulandı. ⚠️ Açık: -999'un ekranda "--" olarak görünmesi için Nextion Editor `.HMI` projesi gerekiyor (bu repoda yok).
**Problem:** Hareket halindeki araçta çalışan cihazda watchdog yok.
**Yap:** Task Watchdog Timer'ı etkinleştir, ana döngüde besle. Reset sebebini (`esp_reset_reason()`) açılışta logla. Güvenli durum tanımla: veri bayatsa Nextion'da gösterge tire/kesik gösterilsin, son değer donmuş halde kalmasın.
**Kabul:** Kasıtlı sonsuz döngü oluşturulduğunda cihaz resetleniyor ve sebep logda görünüyor.

### G1.5 — Ani güç kesintisi dayanıklılığı
> ✅ **TAMAMLANDI** (commit `00e815d`) — `src/WiFiServerModule.cpp`. Doğrudan flash/NVS yazması yoktu; `WiFi.persistent(false)` eksikti, gizli NVS yazmasına yol açıyordu, düzeltildi. Build ile doğrulandı.
**Problem:** Kontak kapandığında besleme aniden kesilir. Flash/NVS yazması sırasında kesilirse bozulma olur.
**Yap:** Flash/NVS yazması olup olmadığını tespit et. Varsa: yazma sıklığını azalt, atomik yazma deseni kullan. Yoksa bunu doğrula ve belgele.
**Kabul:** Güç kesme testinde açılış sorunsuz.

---

## FAZ 2 — UDS sağlamlığı

### G2.1 — Negatif yanıt (NRC) işleme
> ✅ **TAMAMLANDI** — `src/HondaCANModule.h` (`nrcName()`, `_nrcCount`), `src/HondaCANModule.cpp` (rx döngüsünde `0x7F` dalı). `0x78` özel olarak ele alınıyor (istek tekrarlanmadan bekleme uzatılıyor); diğer NRC'ler sayılıp adıyla loglanıyor. Build ile doğrulandı. Not: ECU hangi DID'in reddedildiğini yanıtta echo etmiyor, bu yüzden `0x78` her iki zamanlayıcıyı da öteliyor — DID-bazlı hassas korelasyon G2.2'nin durum makinesiyle gelecek.
**Problem:** Sadece mutlu yol kodlanmış görünüyor.
**Yap:** `0x7F` yanıtlarını ayrıştır ve NRC koduna göre davran. Özellikle **`0x78` (responsePending)**: bekleme süresini uzat, isteği tekrarlama. Diğer NRC'leri say ve logla.
**Kabul:** Bilinmeyen bir DID istendiğinde sistem NRC'yi doğru raporluyor ve kilitlenmiyor.

### G2.2 — Tek istek kuralı ve zaman aşımı
> ✅ **TAMAMLANDI** — `src/HondaCANModule.h` (`UdsRequestState` enum, `DidSlot` struct/dizisi, zaman aşımı/skip sabitleri), `src/HondaCANModule.cpp:136-180` (durum makinesi: IDLE→REQUEST_SENT→WAITING→COMPLETE/TIMEOUT), `:218-246` (rx yanıtlarının FSM'e bağlanması). Önceden RPM (50ms) ve yavaş DID rotasyonu (200ms) birbirinden bağımsız, aynı anda birden fazla istek havada olabiliyordu; artık sistemde her an **tek** bekleyen istek var, art arda 5 zaman aşımından sonra o DID 5sn askıya alınıyor. Build ile doğrulandı. Not: RPM cadence'i artık başka bir DID'in yanıtını beklerken en fazla ~100ms gecikebilir (G0.1 enstrümantasyonuyla ölçülebilir).
**Problem:** UDS istek/yanıt protokolüdür. Aynı anda birden fazla istek uçarsa yanıtlar karışır.
**Yap:** Açık bir istek durum makinesi: `IDLE → REQUEST_SENT → WAITING → COMPLETE/TIMEOUT`. Aynı anda tek bekleyen istek. Zaman aşımında sınırlı tekrar, sonra o DID'i geçici olarak atla.
**Kabul:** Durum makinesi kod içinde açıkça görünüyor; zaman aşımı sayacı loglanıyor.

### G2.3 — Oturum yönetimi ve kurtarma
> ✅ **TAMAMLANDI** — `src/SystemState.h` (`EngineData.ecuPresent`), `src/HondaCANModule.h` (oturum/ECU takibi alanları), `src/HondaCANModule.cpp:24-25` (begin, `_lastSessionAttemptMs`), `:137-155` (2b/2c blokları: oturum yeniden deneme + ECU varlık türetme), `:230/:238/:250` (`0x50`/`0x62`/NRC yanıtlarında `_lastGoodResponseMs` güncelleme), `src/NextionModule.cpp:47-51` (`t_ecu` göstergesi). Build ile doğrulandı.
> **Değerlendirme (raporun istediği gibi):** Genişletilmiş oturumun (0x10 0x03) bu DID'ler için gerçekten gerekli olup olmadığını **kod okuyarak belirleyemem** — bu, Honda ECU firmware'inin bir davranışı ve raporun kendisi mevcut CAN/UDS okumasını (extended session dahil) "gerçek motorda doğrulandı ✅" olarak işaretlemiş. Bu nedenle **oturum tipini değiştirmedim** (raporun "Yapma" kuralı: doğrulanmış CAN/UDS mantığını yeniden yazma). Eğer varsayılan oturumda da okumanın çalışıp çalışmadığını görmek istersen, `HondaCANModule.cpp` begin()'deki `sendFrame29/11(0x02, 0x10, 0x03)` çağrılarını geçici olarak yorum satırı yapıp gerçek ECU'da test etmen gerekir — bunu senin onayın/ölçümün olmadan kalıcı yapmadım.
> ⚠️ Açık: "ECU YOK" metninin ekranda görünmesi (`t_ecu`) yine Nextion Editor `.HMI` projesi (bu repoda yok) gerektiriyor — G1.4/G2.3'teki diğer sentinel/metin alanlarıyla aynı durum.
**Problem:** Genişletilmiş oturum açma başarısız olursa ne olduğu belirsiz.
**Yap:** Oturum açma başarısızsa yeniden dene; başarısız kalırsa açıkça "ECU yok" durumuna geç ve Nextion'da göster. Tester present zamanlaması ile yeniden bağlanma mantığını netleştir.
**Ayrıca değerlendir ve raporla:** Bu DID'ler için genişletilmiş oturum (`0x10 0x03`) gerçekten gerekli mi, yoksa varsayılan oturum yeterli mi? Gereksizse riski azaltmak için varsayılan oturumda kalmak tercih edilir.
**Kabul:** ECU bağlı değilken sistem açılıyor, çalışıyor, durumu bildiriyor.

### G2.4 — ISO-TP çok çerçeveli yanıt kontrolü
> ✅ **TAMAMLANDI** — `src/HondaCANModule.cpp:205-225`. PCI baytının üst nibble'ı kontrol ediliyor; Single Frame (0x0X) dışındaki her şey (First/Consecutive/Flow Control) açıkça loglanıp çerçeve düşürülüyor, artık `data[1]`'in yanlışlıkla SID sanılıp `SystemState`'e sessizce bozuk veri yazılması engellendi. Çok çerçeveli yeniden birleştirme (reassembly) hâlâ desteklenmiyor — mevcut DID'lerin hepsi zaten tek çerçeveye sığıyor, bu sadece gelecekte sessiz bozulmaya karşı bir koruma. Build ile doğrulandı.
**Problem:** Şu anki DID yanıtları tek çerçeveye sığıyor olabilir; 7 bayttan uzun bir yanıt gelirse sessizce bozulur.
**Yap:** Gelen çerçevenin PCI baytını kontrol et. Çok çerçeveli (First Frame) yanıt gelirse ya doğru işle ya da açıkça "desteklenmiyor" olarak logla — sessiz hata bırakma.
**Kabul:** Çok çerçeveli yanıt durumu kodda açıkça ele alınmış.

---

## FAZ 3 — Mimari
*Buradaki değişiklikler için önce seçenek sun, onay bekle.*

### G3.1 — `SystemState` erişim sınırları
> ✅ **TAMAMLANDI** — `src/IModule.h` (ortak `IModule` + `IProducerModule::update(SystemState&)` + `IConsumerModule::update(const SystemState&)`), `HondaCANModule.h`/`IMUModule.h`/`BLEServerModule.h`/`MockCANModule.h` → `IProducerModule`, `NextionModule.h/.cpp`/`WiFiServerModule.h/.cpp`/`SerialLoggerModule.h/.cpp` → `IConsumerModule` + `const SystemState&` imzası. `src/main.cpp:65-86` (ayrı `producers[]`/`consumers[]` dizileri), `:163-181` (setup), `:190-223` (loop — üreticiler önce, sonra tüketiciler), `:126-158` (tekrarı azaltan `beginAndLog`/`checkHealthTransition`/`printTimingRow` yardımcıları — refactor geçişi).
> **Not:** Rapor örneği "BLE"yi tüketici olarak listeliyordu ama BLE gerçekte `state.telematics`'e yazıyor (telefon verisi) — bu yüzden BLE `IProducerModule` tarafına alındı, kullanıcıyla netleştirildi. Üretici-önce-tüketici-sonra sıralaması ek gecikme getirmiyor; tersine BLE→Nextion arasındaki önceden var olan bir turluk bayatlık farkını gideriyor (BLE eskiden Nextion'dan sonra çalışıyordu).
> Build ile doğrulandı (her iki ortam: gerçek + mock), derleyici tüketici modüllerin `state`'e yazma girişimini derleme hatası olarak yakalıyor (denendi, üç tüketici modülün hiçbiri yazmadığı için sorunsuz derlendi).
**Problem:** `update(SystemState& state)` her modüle tüm sisteme yazma yetkisi veriyor. Modülerlik iddiası burada kırılıyor.
**Yap:** Üretici/tüketici ayrımı öner. Tüketiciler (Nextion, BLE, Wi-Fi, logger) `const SystemState&` alsın; sadece üreticiler (CAN, IMU) yazabilsin.
**Kabul:** Derleyici, tüketici modüllerin yazma girişimini hata olarak yakalıyor.

### G3.2 — Donanım soyutlama katmanı (HAL)
**Problem:** İş mantığı donanıma yapışık: `HondaCANModule` doğrudan TWAI, `IMUModule` doğrudan `Wire`, `NextionModule` doğrudan `HardwareSerial` çağırıyor. Bu yüzden host'ta test yazılamıyor ve `test/` boş.
**Yap:** İnce arayüzler tanımla (`ICanBus`, `II2cBus`, `IUartPort`). Her biri için donanım implementasyonu + mock implementasyonu. **UDS/protokol mantığı TWAI'yi hiç görmesin.**
**Öncelik:** Önce sadece `ICanBus` — en değerli ve en test edilebilir olan o. Diğerleri sonra.
**Kabul:** UDS protokol mantığı mock CAN üstünde, donanım olmadan çalışıyor.

> 🔁 **İKİ AŞAMADA TAMAMLANDI.** İlk denemede `HondaCANModule`'ü `ICanBus&` almaya çevirmiştim; kullanıcı o an doğrulanmış CAN alım katmanına dokunulmasını istemedi ("normal data aldığımız katmanı değiştirme... ayrı bir modül ile mock data serve edebilirsin"), bu yüzden tamamen geri alındı ve onun yerine `src/MockCANModule.h/.cpp` (G3.2-alt, `HondaCANModule`'den bağımsız, `IModule` uygulayan ayrı bir modül; `[env:esp32-s3-devkitc-1-mock]`) eklendi.
> ✅ **Sonra kullanıcı G5.1 için gerçek protokol testi ihtiyacını görünce onayladı:** "gerçek CAN katmanını yazmamız lazım, mock katmanı da olması engel değil... main modül içerisinde katmanı switch edebiliriz." Bunun üzerine orijinal `ICanBus` HAL yaklaşımı **bilinçli olarak, onayla** tekrar hayata geçirildi: `src/hal/ICanBus.h` + `TwaiCanBus.h/.cpp` (gerçek), `HondaCANModule` artık `ICanBus&` alıyor (protokol mantığı tek satır değişmeden), `main.cpp` gerçek build'de `TwaiCanBus`'ı enjekte ediyor. `MockCANModule` (G3.2-alt) ve yeni `MockCanBus` (ICanBus test double, G5.1) **ikisi de duruyor, çakışmıyor** — biri ESP32 donanımında sahte veri simülasyonu için, diğeri native ortamda gerçek protokol mantığını test etmek için. Ayrıntılar için G5.1'e bak.

### G3.3 — Paket formatı tek kaynak
**Problem:** 12 baytlık BLE paketi üç yerde ayrı tanımlı: C++ struct, `app.js` ayrıştırıcı, Dart modeli. Biri değişince diğerleri sessizce bozulur.
**Yap:** Formatı tek bir şema dosyasında (JSON veya basit tablo) belgele; üç implementasyonun da o şemayı referans aldığını yorum satırıyla işaretle. Pakete **`version` (1 bayt)** ve **`seq` (1 bayt)** alanları ekle.
**Kabul:** Sürüm uyuşmazlığında istemci paketi reddediyor; `seq` ile paket kaybı ölçülebiliyor.

> ✅ **TAMAMLANDI** — `docs/ble_telemetry_packet_schema.json` (tek kaynak, kullanıcı isteğiyle Markdown değil **JSON** — ileride TinyML eğitim verisi hattı için makine tarafından okunabilir olması gerekiyor). Pakete `version`+`seq` en başa eklendi (13→**15 bayt**, ⚠️ kırıcı protokol değişikliği, üç taraf da bu turda yeniden build edildi):
> - `src/BLETelemetryPacket.h` — `BLE_PACKET_VERSION` tanımı + yeni alanlar + şemaya referans yorumu.
> - `src/BLEServerModule.h/.cpp` — `_txSeq` sayacı, her notify'da `version`/`seq` set ediliyor.
> - `mobile_app/app.js` — `BLE_PACKET_EXPECTED_VERSION`/`BLE_PACKET_SIZE_BYTES`, `appState.packetStats` (sürüm reddi + `seq` boşluğundan kayıp paket sayacı, 0-255 dönüşünü hesaba katarak).
> - `mobile_app/flutter_app/lib/models/telemetry_data.dart` — aynı mantık, statik `TelemetryData.receivedCount/lostCount/versionRejectedCount` sayaçları.
> **Doğrulama:** ESP32 tarafı `pio run` (her iki ortam) ile; C struct offsetleri `offsetof()` ile host'ta gcc kullanılarak doğrulandı (sizeof=15, offsetler 0,1,2,4,5,6,7,9,11,13 — şema ve her iki mobil implementasyonla birebir eşleşiyor); `app.js` hem `node --check` hem bilinen sabit değerlerle sahte paket kodlayıp elle decode ederek doğrulandı. `test/` klasöründe pakete bağlı hardcoded byte dizisi olmadığı için mevcut testler etkilenmedi.

### G3.4 — Zamanlama mantığının merkezileştirilmesi
**Problem:** Her modül kendi `millis()` karşılaştırmasını yapıyor; zamanlama politikası dağılmış.
**Yap:** `IModule`'e periyot bilgisi ekle; çizelgeleme `main.cpp`'de tek yerde yapılsın.
**Not:** FreeRTOS task ayrımı **bu fazda yapılmayacak.** G0.1 ölçümü gerçekten sorun gösterirse ayrı bir tur olarak ele alınır.

> ✅ **TAMAMLANDI** — `src/IModule.h:39` (`virtual uint32_t getPeriodMs() const { return 0; }`, varsayılan = her turda çalıştır). Önce her modülü analiz ettim: sadece **NextionModule** (100ms) ve **SerialLoggerModule** (`_printIntervalMs`) güvenle main.cpp'ye taşınabilecek, kendi içinde başka bir kademeli zamanlaması olmayan saf periyodik modüller. CAN/IMU/WiFi/BLE **dokunulmadı** (dönem 0 kaldı) — her turda çalışmaları gerekiyor (CAN rx kuyruğu, IMU örnekleme hızı, WiFi HTTP yanıt hızı, BLE kuyruk boşaltma) ve zaten kendi iç UDS/protokol zamanlamaları var (G1.3-G2.4'te sertleştirilmiş, dokunulmayacak kod).
> - `src/NextionModule.h/.cpp` — `getPeriodMs()` override (100), `_lastRender` iç kontrolü kaldırıldı.
> - `src/SerialLoggerModule.h/.cpp` — `getPeriodMs()` override (`_printIntervalMs`), `_lastPrint` iç kontrolü kaldırıldı.
> - `src/main.cpp:165-171` (`isDue()` yardımcısı), `:247` (üretici döngüsü), `:260` (tüketici döngüsü) — zamanlama artık tek yerde, modülün `getPeriodMs()` beyanına göre.
> **Doğrulama:** Her iki ortam (`esp32-s3-devkitc-1`, `-mock`) temiz build oldu.

---

## FAZ 4 — Güvenlik

### G4.1 — Wi-Fi SoftAP sıkılaştırma
**Problem:** Açık AP + şifresiz HTTP. Menzildeki herkes telemetriyi okur.
**Yap:** WPA2 + parola. Parola kaynak koda gömülmesin (NVS veya build flag). AP'yi varsayılan kapalı yap, kullanıcı isteğiyle açılsın.
**Kabul:** Parolasız bağlantı reddediliyor.

> ✅ **TAMAMLANDI** (embedded-systems subagent'ı uyguladı, ben build ile bağımsız doğruladım) — Tespit: kod zaten WPA2 kullanıyordu (`AP_PASS "HondaCL250"`, 10 karakter), asıl eksik parolanın kaynak kodda gömülü olması ve AP'nin varsayılan açık olmasıydı.
> - `platformio.ini` — `[platformio] extra_configs = platformio_local.ini`; mock ortamı `build_flags = ${env:esp32-s3-devkitc-1.build_flags} -D MOCK_CAN_DATA` ile flag'i miras alıyor.
> - `platformio_local.ini.example` (commit'li şablon) + `.gitignore`'a `platformio_local.ini` eklendi — gerçek parola artık repoda yok, her geliştirici kendi yerel dosyasını oluşturuyor.
> - `src/WiFiServerModule.cpp:4-8` — `AP_PASSWORD` tanımlı değilse derleme `#error` ile durur (bağımsız doğruladım: dosyayı geçici kaldırınca build FAILED, geri koyunca SUCCESS).
> - `src/WiFiServerModule.h:20-27`, `.cpp:13-46,72-96` — AP artık `begin()`'de açılmıyor; iki tetikleyiciden biri gerçekleşene kadar kapalı kalıyor: **(1)** açılıştan sonraki ilk ~1sn içinde BOOT tuşuna (GPIO0, ek donanım yok) basılı tutma, **(2)** BLE 15sn içinde bağlanamazsa otomatik açılma (`state.telematics.phoneConnected` — mevcut `app.js`/Dart "BLE başarısız → WiFi fallback" akışını hiç bozmuyor). Açıldıktan sonra tekrar kapanmıyor (kasıtlı, basitlik için).
> **Doğrulama:** Her iki ortam (`esp32-s3-devkitc-1`, `-mock`) benim tarafımdan da temiz build edildi; `#error` testi bağımsız tekrarlandı.

### G4.2 — BLE erişim kontrolü
**Problem:** Telematik yazma karakteristiği korumasızsa yabancı biri gidon ekranına yazı yazabilir.
**Yap:** Yazma karakteristiğine eşleştirme/şifreleme (bonding) şartı. Okuma/bildirim açık kalabilir.
**Kabul:** Eşleşmemiş cihazın yazma denemesi reddediliyor.

> ✅ **TAMAMLANDI** (embedded-systems subagent'ı uyguladı, ben bağımsız doğruladım) — `src/BLEServerModule.cpp:4-52` (yalnızca bu dosya değişti, `.h`'ye ve G1.2 kuyruk mimarisine dokunulmadı).
> - Proje klasik Bluedroid tabanlı "ESP32 BLE Arduino" kütüphanesini kullanıyor (NimBLE değil) — bu kütüphanede `PROPERTY_WRITE_ENC` diye bir bayrak **yok**; şifreleme zorunluluğu `BLECharacteristic::setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED)` ile ayrı bir GATT-permission katmanında sağlanıyor. Bunu hem subagent hem ben kurulu framework header'larından (`BLECharacteristic.h`, `esp_gatt_defs.h`, `esp_gap_ble_api.h`) okuyarak doğruladık, tahmine dayanmadı.
> - `begin()`'de `BLESecurity` ile bonding zorunlu kılındı: `ESP_LE_AUTH_REQ_SC_BOND` (secure connections + bonding) + `ESP_IO_CAP_NONE` (kartın ekranı/klavyesi yok → "Just Works" eşleşmesi — MITM koruması yok ama amaç zaten sadece eşleşmemiş yabancı cihazları engellemek, aktif saldırgana karşı değil; bu net bir kabul edilmiş sınır, `SECURITY.md`'de de yazılacak).
> - RX (yazma) karakteristiğine `ESP_GATT_PERM_WRITE_ENCRYPTED` eklendi (bilerek düz `ESP_GATT_PERM_WRITE` ile OR'lanmadı, yoksa şifresiz yazmaya da izin verirdi). TX (notify) hiç dokunulmadı, açık kaldı.
> **Doğrulama:** Her iki ortam benim tarafımdan bağımsız temiz build edildi; ayrıca kullanılan tüm sabitlerin (`ESP_GATT_PERM_WRITE_ENCRYPTED`, `ESP_LE_AUTH_REQ_SC_BOND`, `ESP_IO_CAP_NONE`) gerçekten kurulu framework header'larında var olduğunu ben de ayrıca grep ile teyit ettim.
> ⚠️ **Kabul kriterinin tam doğrulaması** ("eşleşmemiş cihazın yazma denemesi reddediliyor") **sadece gerçek telefon + gerçek ESP32-S3 donanımıyla** yapılabilir — BLE bonding/şifreleme stack davranışı bu ortamda simüle edilemez. Flaş sonrası: (a) eşleşmeden yazma denemesinin reddedildiğini, (b) telefon uygulamasının ilk bağlantıda "Pair with Honda-CL250-Telemetry?" isteğiyle karşılaştığını, (c) eşleştikten sonra normal yazmanın çalışmaya devam ettiğini test etmen gerekiyor.

### G4.3 — Girdi doğrulama
**Problem:** BLE ve HTTP üzerinden gelen string'ler (şarkı adı, navigasyon) doğrudan Nextion'a gidiyorsa taşma veya komut enjeksiyonu riski var.
**Yap:** Uzunluk sınırı, karakter filtreleme, Nextion komut sonlandırıcısı (`0xFF 0xFF 0xFF`) içeren girdilerin temizlenmesi.
**Kabul:** Aşırı uzun ve özel karakterli girdiyle sistem kararlı kalıyor.

> ✅ **TAMAMLANDI** — İki gerçek enjeksiyon noktası buldum ve düzelttim:
> - `src/NextionModule.h` (`NEXTION_MAX_TEXT_LEN`=31), `src/NextionModule.cpp:24-40` (`setTxt()`) — telefon üzerinden BLE ile gelen `songTitle`/`artistName` doğrudan Nextion'ın `"..."` ile tırnaklanan komutuna gömülüyordu; içinde `"` varsa tırnaktan kaçılabiliyor, `0xFF 0xFF 0xFF` varsa komut erken sonlandırılıp yeni bir Nextion komutu enjekte edilebiliyordu. Artık `"`, `\`, kontrol karakterleri ve `0xFF` `_` ile değiştiriliyor, uzunluk 31 karakterle sınırlanıyor.
> - `src/WiFiServerModule.cpp` (`jsonEscape()` + `handleTelemetryJson()`) — aynı alanlar JSON'a kaçışsız ekleniyordu; bir `"` her `/api/telemetry` istemcisinin JSON.parse'ını bozardı. Artık `"`/`\` kaçışlanıyor, kontrol/ASCII-dışı baytlar atılıyor.
> - Uzunluk sınırı zaten vardı (`BLEServerModule::onWrite`'taki `sscanf("...%31[^|]...")` formatı) — bu görev karakter filtreleme + terminator temizliğini ekledi.
> **Doğrulama:** Host'ta (`c++ -std=c++11`) gerçek sanitizasyon mantığıyla üç senaryo test edildi: (1) `"` + gömülü `0xFF 0xFF 0xFF` içeren zararlı string → `Hi__n0.val=99___X` (enjeksiyon etkisiz), (2) `"`/`\`/kontrol karakter içeren string → geçerli kaçışlanmış JSON `A\";\\BC`, (3) 40 karakterlik girdi → 31'e kesildi. Her iki ESP32 ortamı da temiz build oldu.

### G4.4 — Bilinen açıklar dokümanı
**Yap:** `SECURITY.md` oluştur: secure boot yok, flash encryption yok, fiziksel erişim koruması yok — bunlar **bilinçli kabul edilmiş prototip sınırları** olarak yazılsın.
**Not:** Bu doküman, ISO 21434 yaklaşımının öğrenci ölçeğindeki karşılığıdır ve mülakatta değerlidir.

> ✅ **TAMAMLANDI** — `SECURITY.md` oluşturuldu. İki bölüm: "Sertleştirilenler" (G3.3, G4.1-G4.3 ve G1.5/G2.x'te yapılan düzeltmelerin bir özet tablosu) ve "Bilinçli kabul edilen sınırlar" (secure boot yok, flash encryption yok, fiziksel erişim koruması yok, OTA yok, BLE Just Works'ün MITM koruması olmadığı, HTTP API'nin ek kimlik doğrulaması olmadığı, CAN/UDS'in araç üreticisinin kendi tasarımıyla aynı şekilde kimlik doğrulamasız olduğu) — her biri için **neden kabul edildiği** ayrı ayrı yazıldı, sadece liste değil gerekçelendirme.

---

## FAZ 5 — Test altyapısı

### G5.1 — UDS protokol birim testleri
**Ön koşul:** G3.2 (ICanBus)
**Yap:** Mock CAN üstünde: DID ayrıştırma, ölçek/offset dönüşümleri, NRC işleme, zaman aşımı, durum makinesi geçişleri. Native ortamda (`pio test -e native`) koşsun.
**Kabul:** `test/` klasörü dolu; testler donanımsız geçiyor.

> ✅ **TAMAMLANDI (G3.2 kullanıcı onayıyla tekrar hayata geçirildi).** Önce geri alınmıştı; kullanıcı sonradan onayladı: "gerçek CAN katmanını yazmamız lazım, aynı zamanda mock katmanı ikisininde olması engel değil... main modül içerisinde verileri aldığımız katmanı switch edebiliriz." Bunun üzerine:
> - `src/hal/ICanBus.h` (arayüz), `src/hal/TwaiCanBus.h/.cpp` (gerçek TWAI implementasyonu, davranış birebir aynı) tekrar oluşturuldu.
> - `HondaCANModule` artık `ICanBus&` alıyor; **protokol mantığı (state machine, NRC, ISO-TP kontrolü, bus-off recovery) tek satır değişmedi** — sadece I/O çağrıları (`twai_transmit/receive/get_status_info/initiate_recovery/start`) arayüz metodlarına yönlendirildi.
> - `src/main.cpp` gerçek build'de `TwaiCanBus canBus(...)` oluşturup `HondaCANModule`'e enjekte ediyor; `MockCANModule` (G3.2-alt, tamamen ayrı) hâlâ mock build ortamında olduğu gibi duruyor — ikisi çakışmıyor.
> - `test/mocks/MockCanBus.h` — sahte CAN veri yolu (rx kuyruğu, tx log, bus-off/recovery çağrı sayaçları).
> - `test/native_stubs/Arduino.h` genişletildi: `Serial`, `delay()`, `min()/max()` eklendi.
> - `test/test_can_protocol/test_main.cpp` — **7 gerçek protokol testi**: RPM decode + ECU-presence gecikmesi, oturum onayı, NRC (0x78-dışı) durumu bozmadan çözme, çok-çerçeve düşürme, bus-off recovery + üstel backoff, recovery sonrası restart, 5 art arda timeout sonrası DID atlama + soğuma sonrası devam.
> **Bug bulundu ve düzeltildi (test yazarken ortaya çıktı):** `HondaCANModule`'deki `static const` üyeler (`RECOVERY_BACKOFF_MAX_MS`, `UDS_MAX_TIMEOUT_MS` vb.) sınıf-dışı tanımları yoktu — bir ODR ihlali. Gerçek Arduino derlemesinde `min()`/`max()` makro olduğu için (referans almadığı için) sorun çıkmıyordu, ama standart `std::min` template'i kullanıldığında (native testlerde olduğu gibi) linker hata veriyordu. Tüm ilgili üyelere `.cpp`'de tanım eklendi — bu, ESP32 build'ini de daha sağlam/taşınabilir hale getiren gerçek bir düzeltme.
> **Ayrıca temizlendi:** `WiFiServerModule::_lastUpdate` ve `MockCANModule::_dropoutStartedMs` — hiç kullanılmayan ölü üye değişkenler, kaldırıldı. `HondaCANModule.h`'deki iki ayrı `public:` bloğu tek bloğa birleştirildi.
> **Doğrulama:** `pio test -e native` → **15/15 test PASSED** (8 eski + 7 yeni). Her iki ESP32 ortamı da (`esp32-s3-devkitc-1`, `-mock`) temiz build oldu, flash boyutu ~600 bayt arttı (vtable dolaylılığı, beklenen).

### G5.2 — Paket serileştirme testleri
**Yap:** `BLETelemetryPacket` serileştirme/ayrıştırma testleri, endianness dahil.
**Kabul:** Bilinen bayt dizileri beklenen değerlere çözülüyor.

> ✅ **TAMAMLANDI** — `test/test_native/test_main.cpp` (4 test: boyut+offset kontrolü `offsetof()` ile, alan değerleri, little-endian bayt düzeni doğrulaması `reinterpret_cast` ile ham bayt okuyarak). `test/native_stubs/Arduino.h` — sadece `millis()`/`test_setMillis()` içeren minimal stub, native platformda `SystemState.h`/`BLETelemetryPacket.h`'i gerçek ESP32 framework'ü olmadan derlemeyi sağlıyor.
> Ayrıca **mevcut ama bozuk olan** `test/test_main.cpp`'yi buldum ve düzelttim: hâlâ G3.3 öncesi 12 baytlık paketi test ediyordu (`TEST_ASSERT_EQUAL(12, sizeof(BLETelemetryPacket))`), G3.3 sonrası bu **başarısız olurdu**. `test/test_embedded/test_main.cpp`'ye taşıdım ve 15 bayt + version/seq'e güncelledim (bu suite donanım gerektiriyor, atölyede `pio test -e esp32-s3-devkitc-1` ile koşulacak).
> **Doğrulama:** `pio test -e native` → **8/8 test PASSED**, donanımsız, Mac'te ~1-6 saniyede çalışıyor.

### G5.3 — CI
**Yap:** GitHub Actions: her push'ta derleme + native testler.
**Kabul:** Repoda yeşil build rozeti var.

> ✅ **TAMAMLANDI** — `.github/workflows/ci.yml`: her push/PR'da (1) `esp32-s3-devkitc-1` build, (2) `esp32-s3-devkitc-1-mock` build, (3) `pio test -e native`. G4.1'in `AP_PASSWORD` build-flag gereksinimini CI'da karşılamak için workflow, commit edilmeyen geçici bir `platformio_local.ini` (sadece build doğrulaması için dummy parola, asla gerçek donanıma yüklenmiyor) oluşturuyor.
> **Doğrulama:** Workflow'un tam reçetesini (placeholder parola oluşturma + 3 adım) yerel olarak, kullanıcının gerçek `platformio_local.ini`'sini geçici yedekleyip birebir simüle ederek çalıştırdım — üçü de başarılı, sonra kullanıcının gerçek parolası geri yüklendi. GitHub Actions'ın kendisi bu ortamdan tetiklenemiyor (CI runner'a erişimim yok), bu yüzden "yeşil rozet" ancak push sonrası GitHub'da görülebilir — ama workflow'un içeriği ve her adımı burada gerçek şekilde test edildi.

---

## FAZ 6 — Doğrulanmamış bileşenler
*Düzeltme değil, ilk kez çalıştırma. Donanım geldiğinde.*

### G6.1 — MPU6050 devreye alma
**Durum:** Kod yazılmış, hiç çalıştırılmamış.
**Yap:** I2C bağlantısı, adres taraması, ham veri okuma, örnekleme hızı doğrulama. Filtreye geçmeden önce **ham veriyi kaydet.**

> ⏸️ **BEKLEMEDE — donanım gerekiyor.** Gerçek MPU6050 I2C hattına bağlı değilken bu görev yapılamaz (I2C adres taraması, ham veri okuma — hepsi fiziksel sensör gerektiriyor). Atölyede yapılacak.

### G6.2 — Lean angle: bilinen tasarım sorunu ⚠️
**Problem:** İvmeölçer, yerçekimi ile merkezkaç ivmesinin **bileşkesini** ölçer. Dengeli bir virajda motosiklet zaten bu bileşkeyle hizalanır — yani tamamlayıcı filtre 45°'lik bir virajda ~0° okur. Bu bir ayar hatası değil, yöntemin sınırı.
**Şimdilik yap:** Bunu `README.md`'de bilinen sınırlama olarak yaz. Ham IMU + CAN hızı verisini senkron kaydet.
**Sonra (Blok 5):** Hız bilgisiyle merkezkaç bileşeni kestirip çıkaran bir yaklaşım veya araç modelli Kalman. Kaydedilmiş gerçek veri olmadan bu tasarlanamaz.

> ✅ **README.md kısmı TAMAMLANDI** — `README.md`'ye "⚠️ Known Limitations" bölümü eklendi: sorunun fiziksel nedeni, neden şimdi düzeltilemeyeceği, Blok 5'te ne yapılacağı (hız-bazlı merkezkaç düzeltmesi veya araç-modelli Kalman) açıkça yazıldı.
> ⏸️ **"Ham IMU + CAN hızı verisini senkron kaydet" kısmı BEKLEMEDE — donanım gerekiyor.** Kayıt yapılabilmesi için önce G6.1 (gerçek IMU bağlantısı) tamamlanmalı; IMU donanımı yokken senkron kayıt mantıksız (kaydedilecek gerçek IMU verisi yok).

### G6.3 — BLE ve Wi-Fi devreye alma
**Yap:** Gerçek telefonla uçtan uca doğrulama. G0.1 enstrümantasyonu açıkken yap — bu yolların döngü zamanlamasına etkisini ölç.

> ⏸️ **BEKLEMEDE — donanım/gerçek telefon gerekiyor.** G4.2 (BLE bonding) ve G4.1 (WiFi AP tetikleyicileri) kod tarafında hazır; uçtan uca doğrulama ancak gerçek telefonla, atölyede yapılabilir.

---

## Öncelik özeti

| Sıra | Faz | Neden |
|---|---|---|
| 1 | Faz 0 | Ölçmeden düzeltme yapılmaz |
| 2 | Faz 1 | Sessiz bozulma ve donma riskleri |
| 3 | Faz 2 | Doğrulanmış tek parçanın sağlamlaştırılması |
| 4 | Faz 5 (G5.1) | G3.2 sonrası hemen |
| 5 | Faz 3 | Mimari — acele edilmez |
| 6 | Faz 4 | Ürünleşmeden önce |
| 7 | Faz 6 | Donanım geldiğinde |

---

## Kapsam dışı

- FreeRTOS'a tam geçiş — G0.1 verisi gerektirmedikçe
- Mobil uygulama yeniden yazımı
- OTA güncelleme
- SD kart / kalıcı kayıt
- Yeni sensör modülleri (GPS, TPMS)
- Yeni özellikler

Bunlar sonraki turlara aittir. Bu tur **mevcut olanı sağlamlaştırma** turudur.
