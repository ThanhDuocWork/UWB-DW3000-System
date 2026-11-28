#pragma once
#include <cstddef>

bool ota_station_ready();
bool ota_staging_allowed();
bool ota_verify_candidate(std::size_t file_size);
void register_ota_fetch();
