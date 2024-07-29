/***********************************************************************************************************************
 Copyright [2020] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.

 This software is supplied by Renesas Electronics America Inc. and may only be used with products of Renesas Electronics Corp.
 and its affiliates (“Renesas”).  No other uses are authorized.  This software is protected under all applicable laws, 
 including copyright laws.
 Renesas reserves the right to change or discontinue this software.
 THE SOFTWARE IS DELIVERED TO YOU “AS IS,” AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT 
 PERMISSIBLE UNDER APPLICABLE LAW,DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES OF 
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE.  TO THE MAXIMUM 
 EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE (OR ANY PERSON 
 OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING, WITHOUT LIMITATION, 
 ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES;
 ANY LOST PROFITS, OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF 
 THE POSSIBILITY OF SUCH LOSS,DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

## Project Overview:
	
This Example Project demonstrates the LVGL graphics library's ability to live render time domain and frequency domain data.
The demo provides buttons for toggling the time domain, frequency domain, and filtered and unfiltered data.

## Hardware Requirements:
Supported RA boards: EK-RA8D1
1 x RA Board
1 x Graphics Expansion Board (P/N RTK7EK6M3B00001BU)
1 x Micro USB cable
(optional)
1 x signal generator
	
## Hardware Connections:
For EK-RA8D1:

Connect Graphic Expandsion Board to J1, please refer to section 8.1 in EK-RA8D1 - User Manual 
https://www.renesas.com/us/en/document/mat/ek-ra6m3g-v1-users-manual?r=1168091
Connect the USB Debug port on EK-RA8D1 to the PC using a micro USB cable.
	
	For EK-RA8D1: 
		Set the configuration switches (SW1) as below to avoid potential failures.
		+-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
		| SW1-1 PMOD1 | SW1-2 TRACE | SW1-3 CAMERA | SW1-4 ETHA | SW1-5 ETHB | SW1-6 GLCD | SW1-7 SDRAM | SW1-8 I3C |
		+-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
		|     OFF     |     OFF     |      OFF     |     OFF    |     OFF    |     OFF    |     ON      |    OFF    |
		+-------------+-------------+--------------+------------+------------+------------+-------------+-----------+

* Connect the USB Debug port on EK-RA8D1 to the PC using a micro USB cable.

