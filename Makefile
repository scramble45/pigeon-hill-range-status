CXX = g++
TARGET = pigeon-hill-range-status
SOURCE = pigeon-hill-range-status.cpp
INSTALL_DIR = $(HOME)/.local/bin
APP_ID = com.github.scramble45.PigeonHillRangeStatus

# Qt6 settings with proper linking flags
CXXFLAGS = -std=c++17 -Wall -O2 -fPIC $(shell pkg-config --cflags Qt6Core Qt6Widgets Qt6Network)
LIBS = $(shell pkg-config --libs Qt6Core Qt6Widgets Qt6Network)
LDFLAGS = -Wl,--no-undefined

# Qt6 moc - check multiple possible locations
MOC_PATHS := /usr/lib/qt6/moc /usr/lib/libexec/moc /usr/lib/x86_64-linux-gnu/qt6/libexec/moc /app/lib/qt6/libexec/moc /usr/lib/qt6/libexec/moc
MOC := $(firstword $(foreach path,$(MOC_PATHS),$(if $(wildcard $(path)),$(path))))
MOC_FILE = pigeon-hill-range-status.moc

# Fallback to system moc if none found
ifeq ($(MOC),)
    MOC = moc
endif

all: $(TARGET)

$(MOC_FILE): $(SOURCE)
	$(MOC) $(SOURCE) -o $(MOC_FILE)

$(TARGET): $(SOURCE) $(MOC_FILE)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

clean:
	rm -f $(TARGET) $(MOC_FILE)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/
	@echo "✅ Installed to $(INSTALL_DIR)/$(TARGET)"

# Flatpak installation targets
install-flatpak: $(TARGET)
	install -D $(TARGET) /app/bin/$(TARGET)
	install -D $(APP_ID).desktop /app/share/applications/$(APP_ID).desktop
	install -D $(APP_ID).appdata.xml /app/share/metainfo/$(APP_ID).appdata.xml
	install -D $(APP_ID).svg /app/share/icons/hicolor/scalable/apps/$(APP_ID).svg

.PHONY: all clean install install-flatpak