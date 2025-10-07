#pragma once

#include <functional>
#include <string>
#include <vector>

class PopupMenu {
public:
    struct Item {
        std::string label;
        std::function<void()> callback;
    };

    void addItem(const std::string& label, std::function<void()> callback);
    void clear();

    // Returns true if an item was clicked
    bool render(int width, int height);

private:
    std::vector<Item> items;
};
