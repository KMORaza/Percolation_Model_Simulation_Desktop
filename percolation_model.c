#include <gtk/gtk.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#define GRID_SIZE 50
#define CELL_SIZE 10

typedef struct {
    bool open;
    bool filled;
} Site;
typedef struct {
    Site grid[GRID_SIZE][GRID_SIZE];
    int n;
    bool is_percolating;
} Percolation;
typedef struct {
    Percolation *perc;
    GtkWidget *drawing_area;
    GtkWidget *speed_scale;
    GtkWidget *start_button;
    GtkWidget *stop_button;
    GtkWidget *pause_button;
    GtkWidget *restart_button;
    guint timeout_id;
    bool running;
    bool paused;
    double speed;
} AppData;
void init_percolation(Percolation *p) {
    p->n = GRID_SIZE;
    p->is_percolating = false;
    for (int i = 0; i < p->n; i++) {
        for (int j = 0; j < p->n; j++) {
            p->grid[i][j].open = false;
            p->grid[i][j].filled = false;
        }
    }
}
void open_site(Percolation *p, int row, int col) {
    if (row < 0 || row >= p->n || col < 0 || col >= p->n) return;
    p->grid[row][col].open = true;
}
void fill_site(Percolation *p, int row, int col) {
    if (row < 0 || row >= p->n || col < 0 || col >= p->n) return;
    if (!p->grid[row][col].open || p->grid[row][col].filled) return;
    p->grid[row][col].filled = true;
    if (row == p->n - 1) p->is_percolating = true;
    fill_site(p, row - 1, col);
    fill_site(p, row + 1, col);
    fill_site(p, row, col - 1);
    fill_site(p, row, col + 1);
}
bool percolates(Percolation *p) {
    return p->is_percolating;
}
gboolean update_simulation(gpointer user_data) {
    AppData *data = (AppData *)user_data;
    if (!data->running || data->paused) return G_SOURCE_CONTINUE;
    Percolation *p = data->perc;
    int row, col;
    do {
        row = rand() % p->n;
        col = rand() % p->n;
    } while (p->grid[row][col].open);
    open_site(p, row, col);
    if (row == 0 || p->grid[row - 1][col].filled || p->grid[row + 1][col].filled ||
        p->grid[row][col - 1].filled || p->grid[row][col + 1].filled) {
        fill_site(p, row, col);
    }
    gtk_widget_queue_draw(data->drawing_area);
    if (percolates(p)) {
        data->running = false;
        gtk_widget_set_sensitive(data->start_button, FALSE);
        gtk_widget_set_sensitive(data->pause_button, FALSE);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}
void draw_grid(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    Percolation *p = data->perc;
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    for (int i = 0; i < p->n; i++) {
        for (int j = 0; j < p->n; j++) {
            if (p->grid[i][j].filled) {
                cairo_set_source_rgb(cr, 0, 0, 1);
            } else if (p->grid[i][j].open) {
                cairo_set_source_rgb(cr, 1, 1, 1);
            } else {
                cairo_set_source_rgb(cr, 0, 0, 0);
            }
            cairo_rectangle(cr, j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
            cairo_rectangle(cr, j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            cairo_set_line_width(cr, 0.5);
            cairo_stroke(cr);
        }
    }
}
void start_simulation(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    if (!data->running) {
        data->running = true;
        data->paused = false;
        data->timeout_id = g_timeout_add(1000 / data->speed, update_simulation, data);
        gtk_widget_set_sensitive(data->start_button, FALSE);
        gtk_widget_set_sensitive(data->pause_button, TRUE);
        gtk_widget_set_sensitive(data->stop_button, TRUE);
    }
}
void stop_simulation(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    if (data->timeout_id) {
        g_source_remove(data->timeout_id);
        data->timeout_id = 0;
    }
    data->running = false;
    data->paused = false;
    gtk_widget_set_sensitive(data->start_button, TRUE);
    gtk_widget_set_sensitive(data->pause_button, FALSE);
    gtk_widget_set_sensitive(data->stop_button, FALSE);
}
void pause_simulation(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    data->paused = !data->paused;
    gtk_button_set_label(button, data->paused ? "Resume" : "Pause");
}
void restart_simulation(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    if (data->timeout_id) {
        g_source_remove(data->timeout_id);
        data->timeout_id = 0;
    }
    init_percolation(data->perc);
    data->running = false;
    data->paused = false;
    gtk_widget_set_sensitive(data->start_button, TRUE);
    gtk_widget_set_sensitive(data->pause_button, FALSE);
    gtk_widget_set_sensitive(data->stop_button, FALSE);
    gtk_button_set_label(GTK_BUTTON(data->pause_button), "Pause");
    gtk_widget_queue_draw(data->drawing_area);
}
void speed_changed(GtkRange *range, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    data->speed = gtk_range_get_value(range);
    if (data->running && !data->paused) {
        if (data->timeout_id) {
            g_source_remove(data->timeout_id);
        }
        data->timeout_id = g_timeout_add(1000 / data->speed, update_simulation, data);
    }
}
void activate(GtkApplication *app, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Percolation Simulation");
    gtk_window_set_default_size(GTK_WINDOW(window), GRID_SIZE * CELL_SIZE + 200, GRID_SIZE * CELL_SIZE + 100);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE); 
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_window_set_child(GTK_WINDOW(window), box);
    data->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(data->drawing_area, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(data->drawing_area), draw_grid, data, NULL);
    gtk_box_append(GTK_BOX(box), data->drawing_area);
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(box), control_box);
    data->speed_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1, 100, 1);
    gtk_range_set_value(GTK_RANGE(data->speed_scale), data->speed);
    gtk_box_append(GTK_BOX(control_box), gtk_label_new("Speed"));
    gtk_box_append(GTK_BOX(control_box), data->speed_scale);
    g_signal_connect(data->speed_scale, "value-changed", G_CALLBACK(speed_changed), data);
    data->start_button = gtk_button_new_with_label("Start");
    gtk_box_append(GTK_BOX(control_box), data->start_button);
    g_signal_connect(data->start_button, "clicked", G_CALLBACK(start_simulation), data);
    data->stop_button = gtk_button_new_with_label("Stop");
    gtk_box_append(GTK_BOX(control_box), data->stop_button);
    gtk_widget_set_sensitive(data->stop_button, FALSE);
    g_signal_connect(data->stop_button, "clicked", G_CALLBACK(stop_simulation), data);
    data->pause_button = gtk_button_new_with_label("Pause");
    gtk_box_append(GTK_BOX(control_box), data->pause_button);
    gtk_widget_set_sensitive(data->pause_button, FALSE);
    g_signal_connect(data->pause_button, "clicked", G_CALLBACK(pause_simulation), data);
    data->restart_button = gtk_button_new_with_label("Restart");
    gtk_box_append(GTK_BOX(control_box), data->restart_button);
    g_signal_connect(data->restart_button, "clicked", G_CALLBACK(restart_simulation), data);
    gtk_window_present(GTK_WINDOW(window));
}
int main(int argc, char **argv) {
    srand(time(NULL));
    Percolation perc;
    init_percolation(&perc);
    AppData data = {
        .perc = &perc,
        .running = false,
        .paused = false,
        .speed = 10.0,
        .timeout_id = 0
    };
    GtkApplication *app = gtk_application_new("org.example.percolation", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}