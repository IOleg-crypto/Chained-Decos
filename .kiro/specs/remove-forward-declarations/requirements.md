# Requirements Document

## Introduction

Ця специфікація описує необхідність повного усунення forward declarations з C++ проєкту гри. Forward declarations часто використовуються для зменшення часу компіляції та розриву циклічних залежностей, але вони можуть ускладнювати розуміння коду, приховувати справжні залежності та створювати проблеми з підтримкою. Мета - замінити всі forward declarations на proper includes або рефакторинг архітектури для усунення залежностей.

## Glossary

- **Forward Declaration**: Оголошення класу або структури без повного визначення (наприклад, `class Player;`)
- **Header File**: Файл з розширенням .h або .hpp, що містить оголошення класів та функцій
- **Include Directive**: Директива препроцесора `#include`, що вставляє вміст іншого файлу
- **Circular Dependency**: Ситуація, коли два або більше класів залежать один від одного
- **Pointer/Reference**: Вказівник або посилання на об'єкт, що дозволяє використовувати forward declaration
- **Interface**: Абстрактний клас, що визначає контракт для реалізації
- **Dependency Injection**: Техніка передачі залежностей ззовні замість створення їх всередині класу
- **Composition**: Побудова складних об'єктів з простіших компонентів через aggregation
- **Builder Pattern**: Патерн проектування для поступового створення складних об'єктів
- **Parameter Object**: Патерн для групування пов'язаних параметрів в окрему структуру
- **Service Locator**: Патерн для централізованого управління та отримання сервісів (Kernel у цьому проєкті)
- **Over-engineering**: Надмірне ускладнення коду через зайві абстракції, patterns та layers
- **YAGNI**: "You Aren't Gonna Need It" - принцип, що закликає не додавати функціональність до її реальної необхідності
- **Module**: Логічно згрупована частина коду з чіткою responsibility (наприклад, Audio, Collision, Model)
- **Bug**: Помилка в коді, що призводить до некоректної поведінки програми
- **Code Consolidation**: Процес об'єднання дублюючого коду в одне місце
- **Compilation Unit**: Окремий .cpp файл разом з усіма включеними headers

## Requirements

### Requirement 1

**User Story:** Як розробник, я хочу ідентифікувати всі forward declarations у проєкті, щоб зрозуміти масштаб проблеми та спланувати рефакторинг.

#### Acceptance Criteria

1. WHEN система сканує всі header files, THE система SHALL знайти всі forward declarations класів та структур
2. WHEN forward declarations знайдені, THE система SHALL класифікувати їх за типом використання (pointer, reference, return type)
3. WHEN аналіз завершено, THE система SHALL створити звіт з переліком файлів та forward declarations
4. WHEN звіт створено, THE звіт SHALL включати інформацію про причини використання forward declarations
5. THE система SHALL ідентифікувати циклічні залежності між класами

### Requirement 2

**User Story:** Як розробник, я хочу замінити прості forward declarations на proper includes, щоб код був більш явним та зрозумілим.

#### Acceptance Criteria

1. WHEN forward declaration використовується тільки для pointer або reference, THE forward declaration SHALL бути замінена на include відповідного header file
2. WHEN include додається, THE include SHALL бути розміщений у правильному порядку згідно з coding standards
3. WHEN заміна виконана, THE код SHALL компілюватися без помилок
4. WHEN заміна виконана, THE час компіляції SHALL не збільшитися значно (максимум 10%)
5. THE система SHALL перевірити відсутність дублювання includes після заміни

### Requirement 3

**User Story:** Як розробник, я хочу розв'язати циклічні залежності через рефакторинг архітектури, щоб уникнути необхідності у forward declarations.

#### Acceptance Criteria

1. WHEN виявлена циклічна залежність, THE система SHALL запропонувати варіанти рефакторингу (interface extraction, dependency inversion)
2. WHEN застосовується interface extraction, THE новий interface SHALL містити тільки необхідні методи
3. WHEN застосовується dependency inversion, THE залежності SHALL бути інвертовані через абстракції
4. WHEN рефакторинг завершено, THE циклічна залежність SHALL бути повністю усунена
5. THE рефакторинг SHALL зберігати всю існуючу функціональність без змін у поведінці

### Requirement 4

**User Story:** Як розробник, я хочу використовувати composition через interfaces для складних класів, щоб уникнути forward declarations та покращити архітектуру.

#### Acceptance Criteria

1. WHEN клас має багато приватних залежностей, THE клас SHALL використовувати dependency injection через interfaces
2. WHEN composition застосовується, THE header SHALL включати тільки interface headers, а не concrete implementations
3. WHEN composition застосовується, THE concrete implementations SHALL бути створені та ін'єктовані ззовні
4. WHEN dependency injection реалізовано, THE клас SHALL залежати від абстракцій, а не від конкретних типів
5. THE composition approach SHALL покращити testability через можливість mock implementations

### Requirement 5

**User Story:** Як розробник, я хочу перемістити складні залежності з headers у implementation files, щоб зменшити coupling та необхідність у forward declarations.

#### Acceptance Criteria

1. WHEN метод використовує складний тип тільки в реалізації, THE складний тип SHALL бути оголошений тільки в .cpp файлі
2. WHEN параметри методу можуть бути замінені на interfaces, THE параметри SHALL використовувати interface types
3. WHEN return type може бути замінений на interface, THE return type SHALL використовувати interface type
4. WHEN залежність перенесена в .cpp, THE header file SHALL не містити include для цієї залежності
5. THE переміщення залежностей SHALL зменшити кількість includes у header files

### Requirement 6

**User Story:** Як розробник, я хочу використовувати interface segregation для розділення великих класів, щоб зменшити залежності та уникнути forward declarations.

#### Acceptance Criteria

1. WHEN клас має багато responsibilities, THE клас SHALL бути розділений на кілька interfaces
2. WHEN interfaces створені, THE кожен interface SHALL представляти одну responsibility
3. WHEN клас використовує інший клас, THE клас SHALL залежати від interface, а не від конкретної реалізації
4. WHEN interface segregation застосована, THE forward declarations SHALL бути замінені на interface includes
5. THE interface segregation SHALL покращити testability та maintainability коду

### Requirement 7

**User Story:** Як розробник, я хочу перевірити, що всі forward declarations усунені, щоб гарантувати досягнення мети рефакторингу.

#### Acceptance Criteria

1. WHEN рефакторинг завершено, THE система SHALL сканувати всі header files на наявність forward declarations
2. WHEN forward declarations знайдені, THE система SHALL створити звіт з переліком залишкових forward declarations
3. WHEN всі forward declarations усунені, THE проєкт SHALL компілюватися без помилок та warnings
4. WHEN верифікація виконана, THE звіт SHALL показувати нуль forward declarations у всіх header files
5. THE фінальна перевірка SHALL підтвердити повну відсутність forward declarations у проєкті

### Requirement 8

**User Story:** Як розробник, я хочу спростити конструктори класів з багатьма параметрами, щоб покращити читабельність та зручність використання коду.

#### Acceptance Criteria

1. WHEN конструктор має більше трьох параметрів, THE конструктор SHALL використовувати Builder pattern або Parameter Object pattern
2. WHEN застосовується Builder pattern, THE Builder SHALL надавати fluent interface для налаштування об'єкта
3. WHEN застосовується Parameter Object, THE Parameter Object SHALL групувати пов'язані параметри в окрему структуру
4. WHEN залежності передаються через конструктор, THE клас SHALL використовувати Kernel service locator для отримання dependencies
5. THE спрощені конструктори SHALL мати максимум три параметри для обов'язкових залежностей

### Requirement 9

**User Story:** Як розробник, я хочу зменшити оверінженеринг у коді, щоб код був простішим, зрозумілішим та легшим у підтримці.

#### Acceptance Criteria

1. WHEN interface має тільки одну реалізацію, THE interface SHALL бути видалений та замінений на concrete class
2. WHEN abstraction не надає реальної цінності, THE abstraction SHALL бути видалена на користь простішого рішення
3. WHEN клас має тільки один метод, THE клас SHALL бути замінений на вільну функцію або lambda
4. WHEN inheritance hierarchy має тільки один рівень, THE inheritance SHALL бути замінена на composition або пряме використання
5. THE код SHALL використовувати найпростіше рішення, що задовольняє вимоги без зайвої складності

### Requirement 10

**User Story:** Як розробник, я хочу виправити всі існуючі баги у коді під час рефакторингу, щоб покращити загальну якість проєкту.

#### Acceptance Criteria

1. WHEN код рефакториться, THE всі виявлені баги SHALL бути задокументовані та виправлені
2. WHEN баг виправляється, THE виправлення SHALL включати коментар з поясненням проблеми
3. WHEN можливо, THE виправлення багів SHALL включати unit test для запобігання регресії
4. WHEN рефакторинг завершено, THE код SHALL не містити відомих багів
5. THE виправлення багів SHALL бути пріоритетом під час рефакторингу кожного модуля

### Requirement 11

**User Story:** Як розробник, я хочу професійно структурувати вміст модулів, щоб код був організований логічно та зрозуміло.

#### Acceptance Criteria

1. WHEN модуль містить багато файлів, THE файли SHALL бути організовані в логічні підпапки (Core, Interfaces, Utils)
2. WHEN файли можуть бути видалені без втрати функціональності, THE файли SHALL бути видалені
3. WHEN логіка дублюється між файлами, THE логіка SHALL бути консолідована в одному місці
4. WHEN модуль має чітку responsibility, THE структура папок SHALL відображати цю responsibility
5. THE кожен модуль SHALL мати зрозумілу структуру з мінімальною кількістю файлів

### Requirement 12

**User Story:** Як розробник, я хочу централізувати доступ до сервісів через Kernel, щоб всі модулі рушія мали єдиний спосіб отримання залежностей.

#### Acceptance Criteria

1. WHEN модуль потребує сервіс, THE модуль SHALL отримувати сервіс виключно через Kernel
2. WHEN сервіс реєструється, THE реєстрація SHALL відбуватися тільки в одному місці через Kernel
3. WHEN модуль ініціалізується, THE модуль SHALL отримувати всі залежності через Kernel::GetService<T>()
4. WHEN пряма передача залежностей використовується, THE передача SHALL бути замінена на Kernel-based access
5. THE всі модулі рушія SHALL бути під'єднані до Kernel для уніфікованого доступу до сервісів

### Requirement 13

**User Story:** Як розробник, я хочу переконатися, що рефакторинг не порушив існуючу функціональність, щоб гарантувати стабільність проєкту.

#### Acceptance Criteria

1. WHEN рефакторинг завершено, THE всі існуючі unit tests SHALL проходити успішно
2. WHEN рефакторинг завершено, THE всі integration tests SHALL проходити успішно
3. WHEN рефакторинг завершено, THE гра SHALL запускатися та працювати без помилок
4. WHEN рефакторинг завершено, THE час компіляції SHALL не збільшитися більше ніж на 15%
5. THE рефакторинг SHALL не змінювати public API класів без необхідності
