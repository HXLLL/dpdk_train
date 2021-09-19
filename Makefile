APPS := ping pong flood helloworld

define FOREACH
	for DIR in $(APPS); do\
		$(MAKE) -C $$DIR; \
	done
endef

.PHONY: build
build:
	$(call FOREACH)