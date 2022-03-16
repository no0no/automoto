uint8_t flag = 0;

struct Longitude {
	char *lon_raw;
	char lon_dg[3];
	char lon_ms[7];
	char *hem_ew;
};

struct Latitude {
	char *lat_raw;
	char *lat_dg[3];
	char *lat_ms[7];
	char *hem_ns;
};

struct Time {
	char *utc_raw;
	char str_utc[8];

	char hour[2];
	char minute[2];
	char second[2];
};

struct Buffers {
	uint8_t buff[255];
	char buff_str[255];
	char nmea_snt[80];
};


void gps_init() {

}

int gps_checksum() {

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

}

void gps_loop() {

	struct Latitude lat;
	struct Longitude lon;
	struct Time time;
	struct Buffers buffs;

	char *raw_sum;
	char sum_num[3];

	uint8_t count = 0;

	while (1) {
		if (flag == 1) {
			continue;
		}
	}
}
