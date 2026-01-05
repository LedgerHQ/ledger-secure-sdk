.PHONY: doc

BUILD_DIR := build

doc: | $(BUILD_DIR)
	@doxygen doc/Doxyfile
	@echo 'HTML doc can be found in '$(BUILD_DIR)/doc/html/index.html

$(BUILD_DIR):
	@mkdir -p $@
