create table if not exists t_storm (
	app_name VARCHAR(255) NOT NULL,
	server_name VARCHAR(255) not NULL,
	service_name VARCHAR(255) not NULL,
	set_name VARCHAR(255) not NULL,
	ip VARCHAR(255) not NULL,
	port INT UNSIGNED not NULL,
	primary key(app_name, server_name, service_name, set_name)
) ENGINE = InnoDB CHARACTER SET utf8;
