# Radar Trilogy SA

Кастомный радар для GTA San Andreas с расширенными возможностями.

<img width="2560" height="1440" alt="Снимок экрана (250)" src="https://github.com/user-attachments/assets/7093eae1-857e-4ed0-a5d2-26e8416d7a5e" />


## Особенности

- Полная замена стандартного радара
- Отображение названия радиостанции
- Поддержка зон-банд
- Расширенные маркеры - по примеру из кода [More Icon](https://gist.github.com/JuniorDjjr/38ba16704a2de63fd7ea31711d6e9e0f)
- Гибкая настройка через конфигурационный файл

## Установка

1. Скопируйте `radar-trilogy-sa.asi` в папку `scripts` вашей GTA SA
2. Убедитесь, что установлен [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
3. Запустите игру

## Требования

- GTA San Andreas (версия 1.0 US)
- [Plugin SDK](https://github.com/DK22Pac/plugin-sdk) (для сборки)
- Visual Studio 2022/2026 

## Сборка

1. Клонируйте репозиторий в папку `plugin-sdk\tools\myplugin-gen\generated\`
2. Установите переменную окружения `PLUGIN_SDK_DIR` с путём к Plugin SDK
3. Откройте `radar-trilogy-sa.sln` в Visual Studio 2026 Insider
4. Соберите проект в режиме Release

## Конфигурация

Файл конфигурации: `radar-trilogy-sa.ini` (создаётся автоматически)

### Параметры

- `Shape` — форма радара (0 = квадрат, 1 = круг)
- `ShowGangZones` — отображение ганг-зон (0 = выкл, 1 = вкл)
- `ModeMoreIcon` — режим иконок
- `CircleSize` — размер радара

## Лицензия

MIT. См. файл [LICENSE](LICENSE).

## Благодарности

- [DK22Pac](https://github.com/DK22Pac) за Plugin SDK
- Сообществу GTA modding за документацию
- [JuniorDjjr](https://gist.github.com/JuniorDjjr) за пример More Icon

