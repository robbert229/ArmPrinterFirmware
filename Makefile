build:
	$(MAKE) -C quicklz
	$(MAKE) -C host
	$(MAKE) -C client

clean:
	$(MAKE) -C quicklz clean
	$(MAKE) -C host clean
	$(MAKE) -C client clean