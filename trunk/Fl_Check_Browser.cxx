#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FL/fl_draw.H>
#include "Check_Browser.h"

/* This uses a cache for faster access when you're scanning the list
either forwards or backwards. */

Check_Browser::cb_item *Check_Browser::find_item(int n) const {
	int i = n;
	cb_item *p = first;

	if (n <= 0 || n > nitems_ || p == 0) {
		return 0;
	}

	if (n == cached_item) {
		p = cache;
		n = 1;
	} else if (n == cached_item + 1) {
		p = cache->next;
		n = 1;
	} else if (n == cached_item - 1) {
		p = cache->prev;
		n = 1;
	}

	while (--n) {
		p = p->next;
	}

	/* Cast to not const and cache it. */

	((Check_Browser *)this)->cache = p;
	((Check_Browser *)this)->cached_item = i;

	return p;
}

int Check_Browser::lineno(cb_item *p0) const {
	cb_item *p = first;

	if (p == 0) {
		return 0;
	}

	int i = 1;
	while (p) {
		if (p == p0) {
			return i;
		}
		i++;
		p = p->next;
	}

	return 0;
}

Check_Browser::Check_Browser(int x, int y, int w, int h, const char *l = 0)
	: Fl_Browser_(x, y, w, h, l) {
	type(FL_SELECT_BROWSER);
	when(FL_WHEN_NEVER);
	first = last = 0;
	nitems_ = nchecked_ = 0;
	cached_item = -1;
}

void *Check_Browser::item_first() const {
	return first;
}

void *Check_Browser::item_next(void *l) const {
	return ((cb_item *)l)->next;
}

void *Check_Browser::item_prev(void *l) const {
	return ((cb_item *)l)->prev;
}

int Check_Browser::item_height(void *) const {
	return textsize() + 2;
}

#define CHECK_SIZE 8

int Check_Browser::item_width(void *v) const {
	fl_font(textfont(), textsize());
	return int(fl_width(((cb_item *)v)->text)) + CHECK_SIZE + 4;
}

void Check_Browser::item_draw(void *v, int x, int y, int, int) const {
	cb_item *i = (cb_item *)v;
	char *s = i->text;
	int size = textsize();
	Fl_Color col = textcolor();
	int cy = y + (size + 1 - CHECK_SIZE) / 2;
	x += 2;

	fl_color(FL_BLACK);
	if (i->checked) {
		fl_polygon(x, cy, x, cy + CHECK_SIZE,
		x + CHECK_SIZE, cy + CHECK_SIZE, x + CHECK_SIZE, cy);
	} else {
		fl_loop(x, cy, x, cy + CHECK_SIZE,
		x + CHECK_SIZE, cy + CHECK_SIZE, x + CHECK_SIZE, cy);
	}
	fl_font(textfont(), size);
	if (i->selected) {
		col = contrast(col, selection_color());
	}
	fl_color(col);
	fl_draw(s, x + CHECK_SIZE + 4, y + size - 1);
}

void Check_Browser::item_select(void *v, int state) {
	cb_item *i = (cb_item *)v;
	i->selected = state;
	if (state) {
		if (i->checked) {
			i->checked = 0;
			nchecked_--;
		} else {
			i->checked = 1;
			nchecked_++;
		}
	}
}

int Check_Browser::item_selected(void *v) const {
	cb_item *i = (cb_item *)v;
	return i->selected;
}

int Check_Browser::add(char *s) {
	return (add(s, 0));
}

int Check_Browser::add(char *s, int b) {
	cb_item *p = (cb_item *)malloc(sizeof(cb_item));
	p->next = 0;
	p->prev = 0;
	p->checked = b;
	p->selected = 0;
	p->text = strdup(s);

	if (b) {
		nchecked_++;
	}

	if (last == 0) {
		first = last = p;
	} else {
		last->next = p;
		p->prev = last;
		last = p;
	}
	nitems_++;

	return (nitems_);
}

void Check_Browser::clear() {
	cb_item *p = first;
	cb_item *next;

	if (p == 0) {
		return;
	}

	new_list();
	do {
		next = p->next;
		free(p->text);
		free(p);
		p = next;
	} while (p);

	first = last = 0;
	nitems_ = nchecked_ = 0;
	cached_item = -1;
}

int Check_Browser::checked(int i) const {
	cb_item *p = find_item(i);

	if (p) return p->checked;
	return 0;
}

void Check_Browser::checked(int i, int b) {
	cb_item *p = find_item(i);

	if (p && (p->checked ^ b)) {
		p->checked = b;
		if (b) {
			nchecked_++;
		} else {
			nchecked_--;
		}
	}
}

int Check_Browser::value() const {
	return lineno((cb_item *)selection());
}

char *Check_Browser::text(int i) const {
	cb_item *p = find_item(i);

	if (p) return p->text;
	return 0;
}

void Check_Browser::check_all() {
	cb_item *p;

	nchecked_ = nitems_;
	for (p = first; p; p = p->next) {
		p->checked = 1;
	}
}

void Check_Browser::check_none() {
	cb_item *p;

	nchecked_ = 0;
	for (p = first; p; p = p->next) {
		p->checked = 0;
	}
}
