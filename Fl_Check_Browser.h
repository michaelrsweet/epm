/* A Check_Browser is a Hold_Browser with a checkbox at the beginning of
each item. */

#ifndef Check_Browser_h
#define Check_Browser_h

#include <FL/Fl.H>
#include <FL/Fl_Browser_.H>

class Check_Browser : public Fl_Browser_ {

	/* required routines for Fl_Browser_ subclass: */

	void *item_first() const;
	void *item_next(void *) const;
	void *item_prev(void *) const;
	int item_height(void *) const;
	int item_width(void *) const;
	void item_draw(void *, int, int, int, int) const;
	void item_select(void *, int);
	int item_selected(void *) const;

	/* private data */

	struct cb_item {
		cb_item *next;
		cb_item *prev;
		char checked;
		char selected;
		char *text;
	};

	cb_item *first;
	cb_item *last;
	cb_item *cache;
	int cached_item;
	int nitems_;
	int nchecked_;
	cb_item *find_item(int) const;
	int lineno(cb_item *) const;

public:
	Check_Browser(int x, int y, int w, int h, const char *l = 0);

	int add(char *s);               // add an (unchecked) item
	int add(char *s, int b);        // add an item and set checked
					// both return the new nitems()
	void clear();                   // delete all items
	int nitems() const { return nitems_; }
	int nchecked() const { return nchecked_; }
	int checked(int item) const;
	void checked(int item, int b);
	void set_checked(int item) { checked(item, 1); }
	void check_all();
	void check_none();
	int value() const;              // currently selected item
	char *text(int item) const;     // returns pointer to internal buffer
};

#endif
