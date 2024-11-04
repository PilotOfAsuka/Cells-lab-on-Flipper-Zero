
#include <stdio.h>
#include <furi.h>
#include <gui/gui.h>
#include <furi_hal.h>
#include <stdlib.h>
#include "utils.h"
#include <gui/view.h>

// Определение структуры клетки
typedef struct {
    int32_t x, y; // Позиция клетки
    int32_t energy; // Уровень энергии
    int32_t dna; // Днк клетки
    int32_t command; // Команда клетки (что у нее на уме так сказать)
} Cell;

// Определение структуры погоды
typedef struct {
    int32_t temp; // Темп.
    int32_t temp_delay; // Значение для изменения температуры каждые N цикла
    int32_t direction_t;
    int32_t hour; // Время суток
    int32_t hour_delay; // Значение для изменения времени каждые N цикла
} Weather;

// Глобальные переменные
int32_t max_cells = 2000; // Максимальное кольво клеток
Cell cells[2000]; // Начальный массив на 1000 клеток
int32_t cell_count = 30; // Начальное количество клеток
Weather weather = (Weather){
    .hour = 0,
    .direction_t = 1,
    .temp = 0,
    .temp_delay = 500,
    .hour_delay = 1000}; // Инициализация погоды и времени
int32_t cycle_count = 0;

// Функция проверки свободной позиции
bool is_position_occupied(int32_t x, int32_t y) {
    for(int32_t i = 0; i < cell_count; i++) {
        if(cells[i].x == x && cells[i].y == y) {
            return true; // Позиция занята
        }
    }
    return false; // Позиция свободна
}

// Функция движения клетки
static void move_cell(int32_t i) {
    int32_t move_x = ((cells[i].dna) % 3) - 1; // -1, 0 или 1
    int32_t move_y = ((cells[i].dna) % 3) - 1; // -1, 0 или 1

    // Обновление позиции с проверкой границ сетки
    int32_t new_x = cells[i].x + move_x;
    int32_t new_y = cells[i].y + move_y;

    // Ограничиваем координаты в пределах сетки
    new_x = (new_x < 0) ? 0 : (new_x > 63) ? 63 : new_x; //
    new_y = (new_y < 0) ? 0 : (new_y > 63) ? 63 : new_y; //

    // Проверка на занятость новой позиции
    if(!is_position_occupied(new_x, new_y)) {
        cells[i].energy -= 1;
        cells[i].x = new_x;
        cells[i].y = new_y;
    }
}

// Функция фотосинтеза
static void photosyntes(int32_t hour, int32_t i) {
    cells[i].energy += (20 - cells[i].y + hour) % 10;
}

// Простая функция изменения времени
static void set_hour() {
    weather.hour = (weather.hour + 1) % 25;
}

// Простая функция темпeратуры
static void set_temp() {
    weather.temp += weather.direction_t;
    // Изменение направления темпeратуры
    if(weather.temp == 10) {
        weather.direction_t = -1;
    } else if(weather.temp == -10) {
        weather.direction_t = 1;
    }
}

// Функция установки погоды
static void set_weather() {
    // Обновляем погоду каждый weather.hour_delay
    if(cycle_count % weather.hour_delay == 0) {
        set_hour(); // Установка времени
    }

    // Обновляем темп. каждый weather.temp_delay
    if(cycle_count % weather.temp_delay == 0) {
        set_temp(); // Установка темп.
    }
    // Сброс счетчика
    if(cycle_count >= 2000) {
        cycle_count = 0;
    }
}

// Функция выполнения действия клетки на основе её "ума" и генетического кода
static void perform_action(int32_t i) {
    // Сбрасываем команду, если она превышает максимальное число действий
    const int32_t max_commands = 10;
    if(cells[i].command >= max_commands) {
        cells[i].command = 0;
    }

    // Определяем действие на основе команды и ДНК клетки
    int32_t action = (cells[i].command + cells[i].dna) % max_commands;

    // Выполняем действие на основе вычисленного значения
    switch(action) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        photosyntes(weather.hour, i); // Фотосинтез
        break;
    case 5:
    case 6:
    case 7:
        move_cell(i); // Движение
        break;
    case 8:
    case 9:
    }

    // Увеличение значения команды клетки для следующего действия
    cells[i].command++;
}

// Функция отнятия энергии
static void subtract_energy(int32_t energy, int32_t i) {
    cells[i].energy -= energy;
}

static void draw_gui(Canvas* canvas) {
    // Устанавливаем шрифт для отображения количества клеток
    canvas_set_font(canvas, FontSecondary);

    // Создаем буферы для хранения строк
    char buffer_cells[20];
    char buffer_temp[20];
    char buffer_hour[20];

    snprintf(buffer_hour, sizeof(buffer_hour), "Hour: %ld", weather.hour);
    snprintf(buffer_temp, sizeof(buffer_temp), "Temp: %ld", weather.temp);
    snprintf(buffer_cells, sizeof(buffer_cells), "Cells: %ld", cell_count);

    // Рисуем строку на экране
    canvas_draw_str(canvas, 80, 8, buffer_cells);
    canvas_draw_str(canvas, 80, 17, buffer_temp);
    canvas_draw_str(canvas, 80, 26, buffer_hour);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 110, 61, "exit");
}

static void check_dead(int32_t i) {
    // Проверка на смерть клетки
    if(cells[i].energy <= 0) {
        // Удаление клетки
        for(int j = i; j < cell_count - 1; j++) {
            cells[j] = cells[j + 1]; // Сдвиг всех клеток влево
        }
        cell_count--; // Уменьшаем количество клеток
        i--; // Возвращаемся назад, чтобы проверить новую клетку на этой позиции
    }
}

static void cell_division(int32_t i) {
    // Если энергии достаточно, клетка делится
    if(cells[i].energy > 100 && cell_count < max_cells) {
        int32_t new_x = cells[i].x + (rand() % 3 - 1);
        int32_t new_y = cells[i].y + (rand() % 3 - 1);
        int32_t new_dna;

        // Ограничиваем координаты в пределах сетки
        new_x = (new_x < 0) ? 0 : (new_x > 63) ? 63 : new_x; //
        new_y = (new_y < 0) ? 0 : (new_y > 63) ? 63 : new_y; //

        if(perform_action_with_chance(10)) {
            new_dna = (cells[i].dna + rand()) % 64;
        } else {
            new_dna = cells[i].dna;
        }

        // Проверка на занятость позиции
        if(!is_position_occupied(new_x, new_y)) {
            cells[cell_count++] =
                (Cell){.x = new_x, .y = new_y, .energy = cells[i].energy / 2, .dna = new_dna};
            cells[i].energy /= 2;
        } else {
            // Если позиция занята, клетка умирает
            cells[i].energy = 0;
        }
    }
}

// Основная логика
static void logic_game() {
    set_weather(); // Функция установки погоды

    // Цикл проходящи по всем клеткам
    for(int32_t i = 0; i < cell_count; i++) {
        subtract_energy(1, i); // Отнемаем энергию у клетки за каждый цикл

        check_dead(i); //Проверка на смерть клетки

        perform_action(i); // Обработка действия клеткой

        cell_division(i); // Проверка готовности деления клетки
    }
}

// Функция обновления дисплея
static void draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);

    logic_game();

    canvas_clear(canvas);

    draw_gui(canvas);

    for(int32_t i = 0; i < cell_count; i++) {
        // Рисуем клетку на экране
        canvas_draw_dot(canvas, cells[i].x, cells[i].y);
    }
}

// Функция обработки ввода для выхода из эффекта
static void input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

// Основная функция приложения
int32_t cells_app_main(void* p) {
    UNUSED(p);

    // Текущий элемент события типа InputEvent
    InputEvent event;

    // Очередь событий
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Инициализация пяти клеток с случайными координатами и начальными значениями энергии
    for(int i = 0; i < cell_count; i++) {
        cells[i] = (Cell){
            .x = rand() % 32,
            .y = rand() % 16,
            .energy = rand() % 100,
            .dna = rand() % 64}; // Случайные начальные координаты и энергия
    }

    // Создаем ViewPort для отображения анимации
    ViewPort* view_port = view_port_alloc();

    // Устанавливаем callback для отрисовки эффекта
    view_port_draw_callback_set(view_port, draw_callback, NULL);

    // Устанавливаем callback для обработки ввода
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Настройка GUI и добавление ViewPort на весь экран
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Основной цикл эффекта
    while(true) {
        // Проверка событий в очереди
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        // Прерывание цикла при нажатии кнопки "назад"
        if(event.key == InputKeyBack) {
            break;
        }
    }

    // Очистка ресурсов
    furi_message_queue_free(event_queue);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}
