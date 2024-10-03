/*
 * Firmware update
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef FW_UPDATE_H_
#define FW_UPDATE_H_

#include "w25q.h"
#include "fws.h"


/**
 * @brief: Update firmware from storage
 * @param mem: struct w25q handle
 * @retval: 0 on success, -1 on failure
 */
int fw_update(struct w25q *mem);

#endif /* FW_UPDATE_H_ */
