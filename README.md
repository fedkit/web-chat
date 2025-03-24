# IU9 C.A. WEB CHAT

C.A. stands for Collarbone Annihilation.

# About

Сделан на летней практике 5-ю первокурсниками из ИУ9-21Б.

# Dependencies

iu9-ca-web-chat использует
- GCC
- [regexis024-build-system](
  https://gitlab.yyyi.ru/collarbone-annihilation/regexis024-build-system
  )
- [libregexis024](
  https://gitlab.yyyi.ru/kme-devline/libregexis024
  )
- [libjsonincpp](
  https://gitlab.yyyi.ru/collarbone-annihilation/libjsonincpp
  )
- [sqlite3](
  https://www.sqlite.org
  )

Сервис так же использует библиотеки engine_engine_number_9 и new_york_transit_line,
размещённые прямо в репозитории.

Работает только на unix системах.

# Compilation

```sh
regexis024_build_system.sh
./building/main bi ./ "absolute/path/to/installation/root"
```
# Usage

Помимо самого бинарника нужен файл с настройками сервиса. Формат настроек: JSON.
Комментарии не поддерживаются. Пример такого файла находится в example/config.json.
Вместе с бинарным фалом так же распространяются ассеты, необходимые для работы сайта.
Их можно найти в папке assets. В настроках (поле `config.assets`) указывается путь до
папки с ассетами. Путь может быть как абсолютным, так и относительным к рабочей директории.
Поле настроек `config.database` указывает как соединиться с базой данных.
Поддерживается только база данных sqlite3. Поддерживается только хранение в файле.
Поле `config.database.file` указывает путь где хранится sqlite база данных.

Перед тем как использовать сервис нужно его проинициализировать (а точнее проинициализировать
базу данных):

`ROOT_PW="<your desired root password>" iu9-ca-web-chat /path/to/config.json initialize`

Переменная окружения `ROOT_PW` читается для устаановки пароля root пользователю раз и навсегда.
Далее можно запускать сервис:

`iu9-ca-web-chat /path/to/config.json run`

Для остановки сервиса киньте ему SIGTERM или SIGINT.

Утилита `iu9-ca-web-chat-admin-cli` позволяет администратору сервиса контролировать его через сокет
(адрес указан в `config["server"]["admin-command-listen"]`).

По адресам `config.server.admin-command-listen` идёт прослушивание так называемых "команд администратора".
iu9cawebchat определяет свой простой протокол для передачи этих команд.
Утилита iu9-ca-web-chat-admin-cli может отправить текст с некой командой на сервер на этот адрес и получить
ответ от сервера.

```shell
iu9-ca-web-chat-admin-cli <server admin-control address> <command text> [<command text> ...]
```

Дополнительные параметры конкатенируются, разделяясь переводом строки.
Команды администратора:

`updateroopw <new root password>` - сменить пароль пользователя с номером 0

`adduser <user nickname> <user name> <user password> <user bio>` - зарегистрировать пользователя сайта 

`8` - остановить сервис

Если нужно ввести пробел или символ `\ ` в любое из этих полей, перед ними нужно поставить `\ `;
Если указать меньше полей, чем нужно, незаполненные поля станут пустыми строками.

Параметры конфигурации `config.lang.whitelist` и `config.lang.force-order` определяют на
какие языки будет локализован сервер, и какие переводы приоритетнее каких.
На данный момент поддерживаются
 - `ru-RU` 
 - `en-US`

Все переводы хранятся в папке `assets/lang`. Для добавления своего перевода нужно форкнуть репозиторий и
сделать копию файла `assets/lang/ru-RU.lang.json` в `assets/lang/XXXXX.lang.json`.

# Список участников

1. [Китанин Фёдор](https://gitflic.ru/user/fed-kit)
2. [Андреев Григорий](https://gitflic.ru/user/biburat)
3. [Зоткин Владимир](https://gitflic.ru/user/vova2006)
4. [Каримов Адель](https://gitflic.ru/user/ra1n)
5. [Яковлев Антон](https://gitflic.ru/user/yakovlevanton)

# Комментарии (для разработчиков)

Зачем писать комментарии в коде, если можно их вынести в отдельные пдф-ки?

- [Документация для разработчиков](
  https://gitlab.yyyi.ru/collarbone-annihilation/iu9-ca-chat-api)

О том как работает всё остальное можно только догадываться.
