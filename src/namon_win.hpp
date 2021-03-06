/**
 *  @file       namon_win.hpp
 *  @brief      Determining applications and their sockets on Windows
 *  @author     Jozef Zuzelka <xzuzel00@stud.fit.vutbr.cz>
 *  @date
 *   - Created: 
 *   - Edited:  23.06.2017 12:12
 */

#pragma once

#include <string>			//	string

#include "netflow.hpp"      //  Netflow




namespace NAMON
{


/*!
 * @brief       Sets mac address of #g_dev interface into #g_devMac
 * @return      False in case of I/O error. Otherwise true is returned.
 */
int setDevMac();
/*!
 * @brief       Finds PID which belongs to Netflow n
 * @param[in]   n    Netflow information
 * @return      False if IP version is not supported or I/O error occured. True otherwise
 */
int getPid(Netflow *n);
/*!
 * @brief       Finds an application name with given PID
 * @param[in]   pid		Process Identification Number
 * @param[out]  appName Found application name and its arguments
 * @return      False if I/O error occured. True otherwise
 */
int getApp(const int pid, std::string &appName);

void cleanWmiConnection();
int connectToWmi();


//#include "namon_win.tpp"


}	// namespace NAMON
