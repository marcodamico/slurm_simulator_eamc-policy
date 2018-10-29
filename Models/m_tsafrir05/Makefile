SHELL=/bin/bash
driver:
	g++ -Wall est_model.cc est_parse.cc est_driver.cc \
	-o est_driver

tgz:
	cd ..; tar -cvzf /tmp/est.tgz \
	m_tsafrir05/*.{cc,hh,gif,html,pdf} \
	m_tsafrir05/Makefile \
	m_tsafrir05/cdfs/*.gif \
	m_tsafrir05/lublin/*.gif \
	m_tsafrir05/osc/*.gif
	mv /tmp/est.tgz .
