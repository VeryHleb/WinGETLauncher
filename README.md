# Winget Installer Launcher

[![Platform](https://img.shields.io/badge/platform-Windows%207%20%7C%208%20%7C%2010%20%7C%2011-blue)]()
[![Language](https://img.shields.io/badge/language-C%20%2B%20Batch-green)]()
[![License](https://img.shields.io/badge/license-MIT-lightgrey)]()

GUI-лаунчер для пакетной установки программ через **winget**.

---

## ⚠️ Важно про Windows 7

**Лаунчер запускается на Windows 7 SP1**, но **установка программ работать не будет**.

Причина: **winget не поддерживает Windows 7** — он требует системные API, которые появились только в Windows 10 (1809). Скрипт `install_apps.bat` на Win7:

- Не найдёт `winget` в системе.
- Попробует установить его через `install_winget.bat`, но winget всё равно не запустится.
- Завершится с кодом `rc=3` и сообщением `[WARN] winget still not available after bootstrap`.

То есть лаунчер **открывается, показывает список программ, рисует прогресс-бар** — но при нажатии «Установить» реальной установки не произойдёт.

**Поддерживаемые системы для установки:** Windows 10 (1809+) и Windows 11.

---

## ✨ Возможности

- 🖥️ **Нативный GUI** на Win32 API — без зависимостей, в один `.exe`
- ✅ **Выбор приложений** через галочки в списке
- 📊 **Прогресс-бар** установки (N из N)
- 📝 **Живой вывод** `.bat` в правую панель
- 🎨 **Стилизация под Windows 7 Aero** (градиенты, glossy-кнопки)
- 📋 **Файл-репозиторий** `repositories.txt` — легко добавлять свои пакеты
- 🚀 **Автозапуск TgWsProxy** (если выбран Telegram)
- 📁 **Логирование** в `logs\` с автоочисткой старше 30 дней
- 🖱️ **Owner-draw кнопки** — подсветка при наведении
- 🔒 **UAC-манифест** — запрос админ-прав при старте

---

## 📁 Структура проекта

```
winget-setup/
├── launcher.exe              ← готовый лаунчер (собирается из launcher.c)
├── install_apps.bat          ← основной скрипт установки
├── 📂 logs/                  ← логи сессий (создаётся автоматически)
└── 📂 source/
    ├── repositories.txt      ← список пакетов
    ├── install_winget.bat    ← bootstrap winget (для систем без него)
    ├── *.appx, *.msix, *.msixbundle
    └── 📂 exe/
        └── TgWsProxy_windows_7_64bit.exe
```

### 📄 Исходники лаунчера

```
Win32Launcher/
├── launcher.c                ← исходник
├── resource.rc               ← ресурсы (иконка, версия)
├── app.manifest              ← UAC-манифест
├── build.bat                 ← сборка
└── ico.ico                   ← иконка приложения
```

---

## 🚀 Как пользоваться

### 1. Скачать готовый `.exe`

Скачайте последний релиз `launcher.exe` из раздела **Releases** и положите его рядом с `install_apps.bat`.

### 2. Запустить

Двойной клик по `launcher.exe` → UAC-запрос → откроется окно со списком приложений.

### 3. Выбрать программы

- ✅ Галочками отметьте нужные программы
- **«Выбрать все»** / **«Снять все»** — быстрое управление
- **«Установить выбранное»** — запуск установки

### 4. Дождаться завершения

- Прогресс-бар внизу покажет `N из N`
- В правой панели — живой вывод `.bat`
- По завершении: **зелёное «Successfully · Установка завершена»**

---

## 📝 Файл `repositories.txt`

Формат строки:

```
winget.id|Отображаемое имя
```

**Пример:**

```
Discord.Discord|Discord
Telegram.TelegramDesktop|Telegram
VideoLAN.VLC|VLC media player
```

**Правила:**

- Строки, начинающиеся с `#` — комментарии
- Пустые строки игнорируются
- Если `Отображаемое имя` пусто — покажется `winget.id`

**Как добавить своё приложение:**

1. Найдите winget ID:
   ```cmd
   winget search имя_программы
   ```
2. Добавьте строку в `repositories.txt`:
   ```
   Vendor.PackageName|Красивое имя
   ```

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
   git clone https://github.com/your-user/winget-installer.git
   cd winget-installer/Win32Launcher
   ```

2. Проверьте компилятор:
   ```cmd
   gcc --version
   windres --version
   ```

3. Соберите:
   ```cmd
   build.bat
   ```

4. Готовый `launcher.exe` появится в той же папке. Скопируйте его в корень проекта.

### Ключевые флаги компиляции

```batch
gcc launcher.c resource.res -o launcher.exe ^
    -mwindows ^
    -municode ^
    -O2 ^
    -s ^
    -static ^
    -lcomctl32 -luser32 -lgdi32 -lshell32 -lmsimg32
```

| Флаг | Что делает |
|---|---|
| `-mwindows` | Оконное приложение (без консоли) |
| `-municode` | Использовать `wWinMain` |
| `-static` | Статическая линковка (нет зависимостей от DLL MinGW) |
| `-lmsimg32` | `GradientFill` для Aero-стиля |

---

## 🖥️ Совместимость

| ОС | Лаунчер запускается | Установка работает |
|---|---|---|
| Windows 11 | ✅ | ✅ |
| Windows 10 (1809+) | ✅ | ✅ |
| Windows 10 (до 1809) | ✅ | ❌ winget не поддерживается |
| Windows 8.1 | ✅ | ❌ winget не поддерживается |
| Windows 7 SP1 | ✅ | ❌ winget не поддерживается |
| Windows Vista | ❌ | ❌ не тестировалось |

**Требования к системе:**

- Windows 7 SP1 или новее (для запуска лаунчера)
- Windows 10 1809+ / Windows 11 (для установки программ)
- Административные права (запрашиваются через UAC)

---

## 📜 Команды winget

**Установка одной программы (вручную):**

```cmd
winget install --id VideoLAN.VLC --silent
```

**Обновление всех пакетов:**

```cmd
winget upgrade --all
```

**Удаление:**

```cmd
winget uninstall --id VideoLAN.VLC
```

---

## ⚠️ Известные ограничения

- **winget не поддерживает Windows 7** — лаунчер запускается, но установка не работает
- **TgWsProxy** устанавливается **только если выбран Telegram**
- **AIMP** в winget имеет битую ссылку (404) — установите вручную
- **`winget install`** требует прав администратора — UAC-манифест это учитывает

---

## 🤝 Вклад в проект

1. Fork репозитория
2. Создайте ветку: `git checkout -b feature/my-feature`
3. Внесите изменения
4. Commit: `git commit -am 'Add my feature'`
5. Push: `git push origin feature/my-feature`
6. Откройте Pull Request

---

## 📄 Лицензия

MIT License — используйте свободно, включая коммерческие проекты.

---

## 🙏 Благодарности

- [Microsoft winget](https://github.com/microsoft/winget-cli) — пакетный менеджер Windows
- [MinGW-w64](https://www.mingw-w64.org/) — компилятор C для Windows
- [niXman/mingw-builds-binaries](https://github.com/niXman/mingw-builds-binaries) — сборки MinGW с msvcrt

---

## 📞 Контакты

- **Issues**: [GitHub Issues](https://github.com/your-user/winget-installer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-user/winget-installer/discussions)
