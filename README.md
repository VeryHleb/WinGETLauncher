# Winget Installer Launcher

[![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-blue)]()
[![Language](https://img.shields.io/badge/language-C%20%2B%20Batch-green)]()
[![Version](https://img.shields.io/badge/version-1.2.7%20windows%2010-orange)]()
[![License](https://img.shields.io/badge/license-MIT-lightgrey)]()

GUI-лаунчер для пакетной установки программ через **winget** (Windows 10 / 11)
с автоматическим fallback на **chocolatey** для старых систем.

**Последняя стабильная версия: `1.2.7 windows 10`**

---

## ✨ Возможности

- 🖥️ **Нативный GUI** на Win32 API — без зависимостей, в один `.exe`
- 🧠 **Автоопределение ОС и пакетного менеджера:**
  `winget` для Windows 10 (1809+) и Windows 11,
  `chocolatey` для Windows 7 SP1 / 8.1 / старых Windows 10
- 🔁 **Fallback `winget` → `msstore`** — если пакета нет в winget, лаунчер автоматически пробует Microsoft Store
- 🍫 **Автоустановка Chocolatey** — из локального `source\chocolatey\chocolatey.zip` через встроенный `7z.exe`
- ✅ **Выбор приложений** через галочки в списке
- 📂 **Категории приложений** — «Общение», «Игры», «Мультимедиа», «Утилиты» и др.
- 🔍 **Поиск через winget** — отдельное окно с результатами и кнопкой «Добавить в репозиторий»
- 🔄 **Автообновление через GitHub** — сравнение `version.txt`, скачивание файлов по списку `source/files.txt`
- 📊 **Прогресс-бар N из N** — точно по числу выбранных пакетов
- 📝 **Живой вывод** `.bat` в правую панель
- 🎨 **Стилизация под Windows 7 Aero** — градиентный фон, glossy-кнопки, Segoe UI
- 📋 **Файл-репозиторий** `repositories.txt` — легко добавлять свои пакеты
- 🚀 **Автозапуск TgWsProxy** — если выбран Telegram
- 📁 **Логирование** в `logs\` с автоочисткой старше 30 дней
- 🖱️ **Owner-draw кнопки** — подсветка при наведении
- 🔒 **UAC-манифест** — запрос админ-прав при старте
- 🐛 **Отладочный лог** `update_debug.log` — для диагностики обновлений

---

## 🖥️ Поддерживаемые системы

| ОС | Лаунчер | Пакетный менеджер | Установка |
|---|---|---|---|
| **Windows 10 (1809+)** | ✅ | **winget** | ✅ **полная поддержка** |
| **Windows 11** | ✅ | **winget** | ✅ **полная поддержка** |
| Windows 10 (до 1809) | ✅ | Chocolatey | ⚠️ ограниченная |
| Windows 8.1 | ✅ | Chocolatey | ⚠️ ограниченная |
| Windows 7 SP1 | ✅ | Chocolatey | ⚠️ ограниченная (требует PowerShell 3.0+) |
| Windows Vista | ❌ | — | ❌ |

**Основная целевая ОС — Windows 10 (1809+) и Windows 11.**

---

## ⚠️ Важно про Windows 7

**Лаунчер запускается на Windows 7 SP1**, но **`winget` не работает** на этой ОС.

Лаунчер **автоматически переключается на Chocolatey**:

- Если `choco` установлен — использует его.
- Если нет — **устанавливает автоматически** из `source\chocolatey\chocolatey.zip`.
- В заголовке окна будет: `WinGET Launcher — Chocolatey [Windows 6.1 (build 7601)]`.

⚠️ **Ограничения Win7:**
- **Свежий Chocolatey требует PowerShell 3.0+.** На чистой Win7 SP1 стоит PowerShell 2.0.
- Для полноценной работы Chocolatey на Win7 нужен **WMF 5.1** ([скачать](https://aka.ms/wmf5download)).
- Многие пакеты в community-репозитории требуют **TLS 1.2**, который на старом .NET 4.0 не работает без патчей.

Приложения, для которых в `repositories.txt` **не указан** `choco.id`,
на Windows 7 **не появятся в списке** (это ожидаемое поведение).

---

## 📁 Структура проекта

WinGETLauncher/
├── launcher.exe ← готовый лаунчер (собирается из launcher.c)
├── install_apps.bat ← основной скрипт установки
├── version.txt ← текущая версия (без BOM)
├── README.md
├── CHANGELOG.md
├── LICENSE
├── .gitignore
├── 📂 logs/ ← логи сессий (создаётся автоматически)
└── 📂 source/
├── files.txt ← список файлов для автообновления
├── repositories.txt ← список пакетов
├── install_winget.bat ← bootstrap winget
├── search_winget.bat ← обёртка для поиска
├── search_winget.ps1 ← сам поиск
├── *.appx, *.msix, *.msixbundle
├── 📂 chocolatey/
│ └── chocolatey.zip ← переименованный chocolatey.nupkg
├── 📂 7zip/
│ └── 7z.exe ← для распаковки chocolatey.zip
└── 📂 exe/
└── TgWsProxy_windows_7_64bit.exe

### 📄 Исходники лаунчера

Win32Launcher/
├── launcher.c ← исходник (~1200 строк)
├── resource.rc ← ресурсы (иконка, версия)
├── app.manifest ← UAC-манифест
├── build.bat ← сборка
└── ico.ico ← иконка приложения

---

## 🚀 Как пользоваться

### 1. Скачать готовый `.exe`

Скачайте последний релиз `launcher.exe` (версия **1.2.7 windows 10**) из раздела **Releases**
и положите его рядом с `install_apps.bat`.

### 2. Запустить

Двойной клик по `launcher.exe` → UAC-запрос → откроется окно со списком приложений.

### 3. Выбрать программы

- ✅ Галочками отметьте нужные программы
- **«Выбрать все»** / **«Снять все»** — быстрое управление
- **«Поиск winget»** — найти и добавить новые пакеты
- **«Открыть репозиторий»** — отредактировать список вручную

### 4. Установить

- **«Установить выбранное»** — запуск установки
- Прогресс-бар внизу покажет `N из N`
- В правой панели — живой вывод `.bat`
- По завершении: **зелёное «Successfully · Установка завершена»**

---

## 📝 Файл `repositories.txt`

Формат строки:

winget.id|choco.id|Отображаемое имя

**Пример:**

Discord.Discord|discord|Discord
Telegram.TelegramDesktop|telegram|Telegram
VideoLAN.VLC|vlc|VLC media player

**Правила:**

- Строки, начинающиеся с `#` — комментарии
- Пустые строки игнорируются
- Секции `[Категория]` создают заголовок группы
- Если `choco.id` пуст — пакет **не будет установлен** на Windows 7
- Если имя пусто — покажется `winget.id`
- **Источник определяется автоматически:** сначала winget, потом msstore

**Как добавить своё приложение:**

1. Найдите winget ID:
   ```cmd
   winget search имя_программы

2. Найдите Chocolatey ID (если нужна поддержка Win7):
   ```cmd
   choco search имя_программы

3. Добавьте строку:

Vendor.PackageName|choco-name|Красивое имя
text


Или просто нажмите **«Поиск winget»** в лаунчере → выберите результат → **«Добавить в репозиторий»**.

---

## 🔄 Автообновление

Лаунчер умеет обновлять **свои скрипты** с GitHub:

1. При запуске читается `version.txt` из репозитория `VeryHleb/WinGETLauncher`.
2. Если версия отличается — предлагается обновление.
3. Скачивается файл `source/files.txt` со списком файлов.
4. Каждый файл скачивается и сохраняется по нужному пути.
5. Локальный `version.txt` обновляется **без BOM**.

Кнопка **«Проверить обновления»** — ручной запуск.

### 📝 Файл `source/files.txt`

Список файлов для автообновления (один на строку, комментарии через `#`):

install_apps.bat
version.txt
source/repositories.txt
source/install_winget.bat
source/search_winget.bat
source/search_winget.ps1
text


⚠️ **Добавляйте новые файлы в `files.txt`**, иначе автообновление их не увидит.

### 🐛 Диагностика

Если что-то не сработало — смотрите `update_debug.log` рядом с `launcher.exe`:

=== UpdateThread started ===
Local version: [1.2.7 windows 10]
Remote version: [1.2.8 windows 10]
Downloading files list: https://raw.githubusercontent.com/.../source/files.txt
Downloading: .../install_apps.bat
-> D:...\install_apps.bat
OK
...
=== Total: 6 ok, 0 failed ===
text


---

## 🔨 Сборка из исходников

### Требования

- **MinGW-w64 с msvcrt** (не UCRT!) — для совместимости с Windows 7
  - Скачать: [niXman/mingw-builds-binaries](https://github.com/niXman/mingw-builds-binaries/releases)
  - Ищите файл с `msvcrt` в названии
- **`windres`** (входит в MinGW-w64)
- **`gcc`** в `PATH`

### Шаги

1. Клонируйте репозиторий:
   ```cmd
   git clone https://github.com/VeryHleb/WinGETLauncher.git
   cd WinGETLauncher

    Проверьте компилятор:
    cmd

    gcc --version
    windres --version

    Соберите:
    cmd

    build.bat

    Готовый launcher.exe появится в той же папке.

Ключевые флаги компиляции
batch

gcc launcher.c resource.res -o launcher.exe ^
    -mwindows ^
    -municode ^
    -O2 ^
    -s ^
    -static ^
    -lcomctl32 -luser32 -lgdi32 -lshell32 -lmsimg32 -lwininet

Флаг	Что делает
-mwindows	Оконное приложение (без консоли)
-municode	Использовать wWinMain
-static	Статическая линковка (нет зависимостей от DLL MinGW)
-lmsimg32	GradientFill для Aero-стиля
-lwininet	WinINet для скачивания обновлений
📜 Команды winget / choco

Установка одной программы (вручную):
cmd

:: winget
winget install --id VideoLAN.VLC --silent

:: Chocolatey
choco install vlc -y

Обновление всех пакетов:
cmd

:: winget
winget upgrade --all

:: Chocolatey
choco upgrade all -y

Удаление:
cmd

:: winget
winget uninstall --id VideoLAN.VLC

:: Chocolatey
choco uninstall vlc -y

⚠️ Известные ограничения

    launcher.exe не обновляется автоматически — скачиваются только скрипты. Для обновления .exe скачайте новый из Releases.

    AIMP в winget имеет битую ссылку (404) — установите вручную или через Chocolatey.

    TgWsProxy устанавливается только если выбран Telegram.

    Windows 7: свежий Chocolatey требует PowerShell 3.0+ (рекомендуется обновление до WMF 5.1).

    Размер .exe — из-за -static файл весит ~300 КБ (вместо ~50 КБ с UCRT).

🤝 Вклад в проект

    Fork репозитория

    Создайте ветку: git checkout -b feature/my-feature

    Внесите изменения

    Commit: git commit -am 'Add my feature'

    Push: git push origin feature/my-feature

    Откройте Pull Request

📄 Лицензия

MIT License — используйте свободно, включая коммерческие проекты.
🙏 Благодарности

    Microsoft winget — пакетный менеджер Windows

    Chocolatey — пакетный менеджер для старых ОС

    MinGW-w64 — компилятор C для Windows

    niXman/mingw-builds-binaries — сборки MinGW с msvcrt

📞 Контакты

    Issues: GitHub Issues

    Releases: GitHub Releases

    Changelog: CHANGELOG.md