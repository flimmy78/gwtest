#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "socket.h"
#include "proto.h"
#include "sheet.h"
#include "cmd.h"

#include <iostream>


#include <Commctrl.h>
#include "windows.h"

#include "resource.h"

using namespace std;

#define main_printf printf

#define MAX_LOADSTRING 100

enum {
  S_DISCONNECTTED = 0,
  S_CONNECTTED_IDLE = 1,
  S_TESTING = 2,
};

typedef int (*TESTFUNC)(void *arg);

typedef struct TestOption {
  int id;
  const char *name;
  int checked;
  const char *desc;
  TESTFUNC tfunc;
  int retval;
  char strval[256];
}TestOption_t;

typedef struct BoardInfo {
  char mac[32];
  char sysversion[256];
  char zbversion[32];
  char zwversion[32];
  char model[256];
}BoardInfo_t;

static HINSTANCE g_hinst = NULL;
static HWND g_hwnd = NULL;
SOCKET g_sock = NULL;


static int state_convert(int state);
static int target_connect();
static int target_disconnect();
static void test_option_get();
static void target_test();
static void test_write_result();
static void test_info_append(const char *str);
static void test_info_clear();
static void test_select_all();


static int test_lan(void *arg);
static int test_mac(void *arg);
static int test_sysversion(void *arg);
static int test_zbversion(void *arg);
static int test_zwversion(void *arg);
static int test_model(void *arg);
static int test_time(void *arg);

static int test_wan_dhcpcli(void *arg);
static int test_wan_ping_gw(void *arg);
static int test_wan_dns_ping(void *arg);
static int test_wifi_sta(void *arg);
static int test_wifi_ap(void *arg);
static int test_wifi_smartconfig(void *arg);
static int test_4g_usb_device(void *arg);
static int test_4g_at_cmd(void *arg);
static int test_sim_card_chk(void *arg);
static int test_zigbee_pair(void *arg);
static int test_zwave_pair(void *arg);
static int test_ble_dev_exsit(void *arg);
static int test_ble_scan(void *arg);
static int test_btn_pressdown(void *arg);
static int test_led_powerled(void *arg);
static int test_led_zwaveled(void *arg);
static int test_led_zigbeeled(void *arg);
static int test_led_4gled(void *arg);
static int test_led_errorled(void *arg);
static int test_nxp_pair(void *arg);



static int test_write_option_result_string(TestOption_t *to, int ret);

static TestOption_t g_tos[] = {
  {0, "TIME", 0, "", test_time, -1,},
  {TXT_MAC,"MAC", 0, "", test_mac, -1 },
  {TXT_SYSVERSION,"SysVersion", 0, "", test_sysversion, -1 },
  {TXT_ZBVERSION,"ZBVersion", 0, "", test_zbversion, -1 },
  {TXT_ZWVERSION,"ZWVersion", 0, "", test_zwversion, -1 },
  {TXT_MODEL,"Model", 0, "", test_model, -1 },

  { CHK_LAN,"LAN 连接性", 0, "", test_lan, -1,{ 0 } },

  {CHK_WAN_DHCPCLI, "WAN DHCP CLI", 0, "", test_wan_dhcpcli, -1},
  {CHK_WAN_PING_GW, "WAN Ping Gw", 0, "", test_wan_ping_gw, -1},
  {CHK_WAN_DNS_PING, "WAN DNS PING", 0, "",test_wan_dns_ping, -1},

  //{CHK_WIFI_STA, "STA", 0, "", test_wifi_sta, -1},
  //{CHK_WIFI_AP, "AP", 0, "", test_wifi_ap, -1},
  {CHK_WIFI_SMARTCONFIG, "SmartConfig", 0, "", test_wifi_smartconfig, -1},

  {CHK_4G_USB_DEVICE, "4G USB Device", 0, "", test_4g_usb_device, -1},
  {CHK_4G_AT_CMD, "4G AT CMD", 0, "", test_4g_at_cmd, -1},
  {CHK_SIM_CARD_CHK, "SIM CARD CHK", 0, "", test_sim_card_chk, -1},

  {CHK_ZIGBEE_PAIR, "Zigbee Pair", 0, "", test_zigbee_pair, -1},
  {CHK_ZWAVE_PAIR, "ZWave Pair", 0, "", test_zwave_pair, -1},
  
  {CHK_BLE_DEV_EXSIT, "BLE Dev Exsit", 0, "", test_ble_dev_exsit, -1},
  //{CHK_BLE_SCAN, "BLE Scan", 0, "", test_ble_scan, -1},

  {CHK_BTN_PRESSDOWN, "PressDown", 0, "", test_btn_pressdown, -1},

  {CHK_LED_POWERLED, "Power Led", 0, "", test_led_powerled, -1},
  {CHK_LED_ZWAVELED, "ZWave Led", 0, "", test_led_zwaveled, -1},
  {CHK_LED_ZIGBEELED, "Zigbee Led", 0, "", test_led_zigbeeled, -1},
  {CHK_LED_4GLED, "4G Led", 0, "", test_led_4gled, -1},
  {CHK_LED_ERRORLED, "Err Led", 0, "", test_led_errorled, -1 },

  {CHK_NXP_PAIR, "Nxp Pair", 0, "", test_nxp_pair, -1},
};

static BoardInfo_t bi = {
  "Unknown",
  "Unknown",
  "Unknown",
  "Unknown",
  "Unknown",
};

static char g_gwip[32] = "192.168.66.1";
static int g_gwport = 6666;

INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SetIp(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

int state = S_DISCONNECTTED;

int main(int argc, char *argv[]) {
/*
int APIENTRY WinMainCRTStartup(_In_ HINSTANCE hInstance,
		      _In_opt_ HINSTANCE hPrevInstance,
		      _In_ LPWSTR    lpCmdLine,
		      _In_ int       nCmdShow) {
*/

  //UNREFERENCED_PARAMETER(hPrevInstance);
  //UNREFERENCED_PARAMETER(lpCmdLine);

  //g_hinst = hInstance;
  g_hinst = GetModuleHandle(NULL);
  
  int ret = DialogBox(NULL, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
  if (ret == -1) {
    main_printf("Error is %d\n", GetLastError());
  }

  return 0;
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		g_hwnd = hwndDlg;
		socket_init();
		//state_convert(S_DISCONNECTTED);
		state_convert(S_CONNECTTED_IDLE);
		SendMessage(GetDlgItem(g_hwnd, PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(0, 100)); //设置进度条的范围
		SendMessage(GetDlgItem(g_hwnd, PROGRESS), PBS_MARQUEE, 1, 0); //设置PBS_MARQUEE 是滚动效果 
		test_select_all();

		SetDlgItemText(g_hwnd, IDC_IPP, (LPCTSTR)(g_gwip));
		{
		  char buf[32];
		  sprintf(buf, "%d", g_gwport);
		  SetDlgItemText(g_hwnd, IDC_PORTP, (LPCTSTR)buf);
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		socket_uninit();
		return TRUE;

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		int wmEvent = HIWORD(wParam);
		if (wmId == IDM_ABOUT) {
			DialogBox(g_hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), g_hwnd, About);
			return TRUE;
		}
		else if (wmId == IDM_SETIP) {
			DialogBox(g_hinst, MAKEINTRESOURCE(IDD_SETIP), g_hwnd, SetIp);
			return TRUE;
		}
		else if (wmId == IDM_EXIT) {
			EndDialog(hwndDlg, 0);
			socket_uninit();
			return TRUE;
		}

#if 0		
		switch (state) {
		case S_DISCONNECTTED:
			switch (wmId) {
			case BTN_CONNECT:
#if 0
				xls_test();
#else
				if (target_connect() == 0) {
					state_convert(S_CONNECTTED_IDLE);
				}
				else {
					MessageBox(NULL, "Can't connect to server!", "Error", MB_OK);
				}
#endif
				break;
			}
			break;
		case S_CONNECTTED_IDLE:
			switch (wmId) {
			case BTN_DISCONNECT:
				target_disconnect();
				state_convert(S_DISCONNECTTED);
				break;
			case BTN_TEST:
				state_convert(S_TESTING);
				test_option_get();
				target_test();
				state_convert(S_CONNECTTED_IDLE);
				break;
			}
			break;
		case S_TESTING:
			switch (wmId) {
			case BTN_STOP:
				MessageBox(NULL, "Now not support stop!", "Info", MB_OK);
				break;
			}
			break;
		}
#else
		switch (wmId) {
		case BTN_TEST:
		  if (target_connect() == 0) {
		    state_convert(S_TESTING);
		    
		    test_option_get();
		    target_test();
		    //target_disconnect();
		    //state_convert(S_CONNECTTED_IDLE);
		  } else {
		    MessageBox(NULL, "Can't connect to server!", "Error", MB_OK);
		  }
		}
#endif	     
		return TRUE;
	}
	}
	return FALSE;

}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
  case WM_INITDIALOG:
    return (INT_PTR)TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}

INT_PTR CALLBACK SetIp(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == BTN_SETIP) {
			char ip[32];
			GetDlgItemTextA(hDlg, IDC_IP, ip, sizeof(ip));
			char port[32];
			GetDlgItemTextA(hDlg, IDC_PORT, port, sizeof(port));
			strcpy(g_gwip, ip);
			g_gwport = atoi(port);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			//MessageBoxA(NULL, ip, port, 0);
		}
		break;
	}
	return (INT_PTR)FALSE;
}



int target_connect() {
  //g_sock = socket_client_open("192.168.100.103", 6666);
	//g_sock = socket_client_open("192.168.66.1", 6666);
  char ip[32];
  char sport[32];
  GetDlgItemText(g_hwnd, IDC_IPP, ip, sizeof(ip));
  GetDlgItemText(g_hwnd, IDC_PORTP, sport, sizeof(sport));

  strcpy(g_gwip, ip);
  g_gwport = atoi(sport);
  
  g_sock = socket_client_open(g_gwip, g_gwport);
  //g_sock = socket_client_open("127.0.0.1", 9898);
  if (g_sock == NULL) {
    return -1;
  }
  return 0;
}

int target_disconnect() {
  cout << "target disconnect" << endl;
  socket_close(g_sock);
  g_sock = NULL;
  return 0;
}

void test_option_get() {
  int i = 0;
  int cnt = sizeof(g_tos)/sizeof(g_tos[0]);
  int flag = 0;

  for (i = 0; i < cnt; i++) {
    if (g_tos[i].id == CHK_LAN) {
      flag = 1;
    }

    g_tos[i].checked = 1;
    if (flag) {
      g_tos[i].checked = IsDlgButtonChecked(g_hwnd, g_tos[i].id);
    }
  }
}

void _target_test() {
  int i = 0;
  int cnt = sizeof(g_tos) / sizeof(g_tos[0]);

  int dlt = 100 / cnt;

  SendMessage(GetDlgItem(g_hwnd, PROGRESS), PBM_SETPOS, 1, (LPARAM)0);   //设置进度 
  test_info_clear();
  test_info_append("Start...");

  for (i = 0; i < cnt; i++) {
	char buff[256];

    g_tos[i].retval = -1;
    strcpy(g_tos[i].strval, "--");
    if (g_tos[i].checked == 0) {
      continue;
    }

    if (g_tos[i].tfunc != NULL) {
	  sprintf(buff, "======Testing...【%s】======", g_tos[i].name);
	  test_info_append(buff);
      g_tos[i].retval = g_tos[i].tfunc(&g_tos[i]);
    }

 
     //test_write_option_result_string(&g_tos[i], g_tos[i].retval);
  


	if (g_tos[i].retval == 0) {
		sprintf(buff, "【%s】 √", g_tos[i].name);
	}
	else {
		sprintf(buff, "【%s】 ×", g_tos[i].name);
	}
	test_info_append(buff);
    SendMessage(GetDlgItem(g_hwnd, PROGRESS), PBM_SETPOS, i*dlt, (LPARAM)0);   //设置进度 
    //Sleep(100);
	//sprintf(buff, "==========", g_tos[i].name);
	//test_info_append(buff);
  }
  test_info_append("Write Result...");
  test_write_result();
  SendMessage(GetDlgItem(g_hwnd, PROGRESS), PBM_SETPOS, 100, (LPARAM)0);   //设置进度 
  test_info_append("Write End!");

  SendMessage(GetDlgItem(g_hwnd, PROGRESS), PBM_SETPOS, 0, (LPARAM)0);   //设置进度 
  test_info_append("End!");

}
DWORD WINAPI targt_test_thread(LPVOID lpParam) {
  _target_test();
  target_disconnect();
  state_convert(S_CONNECTTED_IDLE);
  return 0;
}
void target_test() {
  CreateThread(NULL, 0, targt_test_thread, NULL, 0, NULL);
}



static int state_convert(int _state) {
  state = _state;
  switch(state) {
  case S_DISCONNECTTED:
    //EnableWindow(GetDlgItem(g_hwnd, BTN_CONNECT), TRUE);
    //EnableWindow(GetDlgItem(g_hwnd, BTN_DISCONNECT), FALSE);
    //EnableWindow(GetDlgItem(g_hwnd, BTN_TEST), FALSE);
    //EnableWindow(GetDlgItem(g_hwnd, BTN_STOP), FALSE);
    break;
  case S_CONNECTTED_IDLE:
    //EnableWindow(GetDlgItem(g_hwnd, BTN_CONNECT), FALSE);
    //EnableWindow(GetDlgItem(g_hwnd, BTN_DISCONNECT), TRUE);
    EnableWindow(GetDlgItem(g_hwnd, BTN_TEST), TRUE);
    EnableWindow(GetDlgItem(g_hwnd, BTN_STOP), FALSE);
    break;
  case S_TESTING:
    //EnableWindow(GetDlgItem(g_hwnd, BTN_CONNECT), FALSE);
    //EnableWindow(GetDlgItem(g_hwnd, BTN_DISCONNECT), FALSE);
    EnableWindow(GetDlgItem(g_hwnd, BTN_TEST), FALSE);
    EnableWindow(GetDlgItem(g_hwnd, BTN_STOP), FALSE);
    break;
  }
  return 0;
}

static int test_time(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  time_t timer;
  struct tm *tblock;
  timer = time(NULL);
  tblock = localtime(&timer);

  strcpy(to->strval, asctime(tblock));

  return 0;
}
static int test_lan(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  int ret = 0;
  return test_write_option_result_string(to, ret);
}

static int test_mac(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_mac(val);
  if (ret == 0) {
	  strcpy(to->strval, val);
	  SetDlgItemTextA(g_hwnd, TXT_MAC, to->strval);
  }


  return ret;
}
static int test_sysversion(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_sysversion(val);
  if (ret == 0) {
	  strcpy(to->strval, val);
	  SetDlgItemTextA(g_hwnd, TXT_SYSVERSION, to->strval);
  }

  return ret;
}
static int test_zbversion(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_zbversion(val);
  if (ret == 0) {
	  strcpy(to->strval, val);
	  SetDlgItemTextA(g_hwnd, TXT_ZBVERSION, to->strval);
  }

  return ret;
}
static int test_zwversion(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_zwversion(val);
  if (ret == 0) {
	  strcpy(to->strval, val);
	  SetDlgItemTextA(g_hwnd, TXT_ZWVERSION, to->strval);
  }

  return ret;
}
static int test_model(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_model(val);
  if (ret == 0) {
	  strcpy(to->strval, val);
	  SetDlgItemTextA(g_hwnd, TXT_MODEL, to->strval);
  }

  return ret;
}

static int test_wan_dhcpcli(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_wan_dhcpcli(val);

  return test_write_option_result_string(to, ret);
}
static int test_wan_ping_gw(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_wan_ping_gw(val);

  return test_write_option_result_string(to, ret);
}
static int test_wan_dns_ping(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_wan_dns_ping(val);

  return test_write_option_result_string(to, ret);
}
static int test_wifi_sta(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_wifi_sta(val);

  return test_write_option_result_string(to, ret);
}
static int test_wifi_ap(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_wifi_ap(val);

  return test_write_option_result_string(to, ret);
}
static int test_wifi_smartconfig(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  MessageBox(g_hwnd, "请启动下手机端的SmartConfig 功能,进行SmartConfig测试, 并迅速点确定按钮!!!", "SmartConfig Test准备", MB_OK);
  char val[128];
  int ret = cmd_request_wifi_smartconfig(val);

  return test_write_option_result_string(to, ret);
}
static int test_4g_usb_device(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_4g_usb_device(val);

  return test_write_option_result_string(to, ret);
}
static int test_4g_at_cmd(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_4g_at_cmd(val);

  return test_write_option_result_string(to, ret);
}
static int test_sim_card_chk(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  char val[128];
  int ret = cmd_request_sim_card_chk(val);

  return test_write_option_result_string(to, ret);
}
static int test_zigbee_pair(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  MessageBox(g_hwnd, "请按下Zigbee测试设备配网按钮,进行zigbee配网测试, 并迅速点确定按钮!!!", "Zigbee Test准备", MB_OK);  
  char val[128];
  int ret = cmd_request_zigbee_pair(val);
  return test_write_option_result_string(to, ret);
}
static int test_zwave_pair(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  MessageBox(g_hwnd, "请按下ZWave测试设备配网按钮 功能,进行zwave配网测试, 并迅速点确定按钮!!!", "ZWave Test准备", MB_OK);
  char val[128];
  int ret = cmd_request_zwave_pair(val);
  return test_write_option_result_string(to, ret);
}
static int test_ble_dev_exsit(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_ble_dev_exsit(val);
  return test_write_option_result_string(to, ret);
}
static int test_ble_scan(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_ble_scan(val);
  return test_write_option_result_string(to, ret);
}
static int test_btn_pressdown(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;

  MessageBox(g_hwnd, "请按下网关的按钮 ,进行按钮测试, 并迅速点确定按钮!!!", "按钮 Test准备", MB_OK);  
  char val[128];
  int ret = cmd_request_btn_pressdown(val);
  return test_write_option_result_string(to, ret);
}
static int test_led_powerled(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_led_powerled(val);
  return test_write_option_result_string(to, ret);
}
static int test_led_zwaveled(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_led_zwaveled(val);
  return test_write_option_result_string(to, ret);
}
static int test_led_zigbeeled(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_led_zigbeeled(val);
  return test_write_option_result_string(to, ret);
}
static int test_led_4gled(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_led_4gled(val);
  return test_write_option_result_string(to, ret);
}
static int test_led_errorled(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  int ret = cmd_request_led_errorled(val);
  return test_write_option_result_string(to, ret);
}
static int test_nxp_pair(void *arg) {
  TestOption_t *to = (TestOption_t *)arg;
  char val[128];
  MessageBox(g_hwnd, "请按下NXP配网按钮3秒后松开 ,进行NXP配网测试, 并迅速点确定按钮!!!", "NXP 配网准备", MB_OK);  
  int ret = cmd_request_nxp_pair(val);
  return test_write_option_result_string(to, ret);
}

static int test_write_option_result_string(TestOption_t *to, int ret) {
	//char buf[64];
	//sprintf(buf, "%d", ret);
	//MessageBoxA(g_hwnd, buf, buf, 0);
  if (ret == -1) {
    strcpy(to->strval, "--");
  }
  else if (ret == 0)  {
    strcpy(to->strval, "√");
  }
  else {
    strcpy(to->strval, "×");
  }
  return ret;
}
static void test_year_day_str(char *ydstr, int len) {

  time_t timer;
  struct tm *tblock;
  timer = time(NULL);
  tblock = localtime(&timer);

  char buff[128];
  memset(ydstr, 0, len);
  sprintf(buff, "%d年%d月%d号-By_%s.xls", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, "Keven");
  
  strcpy(ydstr, buff);
  //MultiByteToWideChar(CP_ACP, 0, buff, strlen(buff), ydstr, len);
}



static void test_write_result() {
  int i = 0;
  int cnt = sizeof(g_tos) / sizeof(g_tos[0]);
  const char *path = ".";
  char name[128];
  test_year_day_str(name, 128);

  const char* cols[256];
  memset(cols, 0, sizeof(char *) * 256);

  for (i = 0; i < cnt; i++) {
    cols[i] = new char[128];
    memset((void *)cols[i], 0, sizeof(char) * 128);
    strcpy((char *)cols[i], g_tos[i].name);
    //MultiByteToWideChar(CP_ACP, 0, g_tos[i].name, strlen(g_tos[i].name), cols[i], 128);
    //wsprintf(cols[i], L"%s", g_tos[i].name);
  }
  BookHandle bh = sheet_open(path, name, cols, cnt);

  if (bh == NULL) {
    for (i = 0; i < cnt; i++) {
      if (cols[i] != NULL) {
	delete cols[i];
      }
    }
    MessageBox(NULL, "Error", "open a.xls failed!", MB_OK);
    for (i = 0; i < cnt; i++) {
      if (cols[i] != NULL) {
	delete cols[i];
      }
    }
    return;
  }

  for (i = 0; i < cnt; i++) {
    memset((void *)cols[i], 0, sizeof(char) * 128);
    strcpy((char *)cols[i], g_tos[i].strval);
    //MultiByteToWideChar(CP_ACP, 0, g_tos[i].strval, strlen(g_tos[i].strval), cols[i], 128);
  }
  sheet_append(bh, (const char **)cols, cnt);
  sheet_save(bh, path, name);
  sheet_close(bh);

  for (i = 0; i < cnt; i++) {
    if (cols[i] != NULL) {
      delete cols[i];
    }
  }
}

static void test_info_append(const char *str) {
  //IDC_INFO
	char buf[256];
	time_t timer;
	struct tm *tblock;
	timer = time(NULL);
	tblock = localtime(&timer);

	sprintf(buf, "[INFO][%02d:%02d:%02d]:%s\r\n", tblock->tm_hour, tblock->tm_min, tblock->tm_sec, str);
	SendMessage(GetDlgItem(g_hwnd, IDC_INFO), EM_SETSEL, -1, 0);
	SendMessageA(GetDlgItem(g_hwnd, IDC_INFO), EM_REPLACESEL, 0, (LPARAM)(LPCSTR)buf);
}

extern void test_info_append_hex(const char *X, char *buf, int len) {
	char str[1024];
	int i = 0;
	int pos = 0;
	pos += sprintf(str + pos, "%s", X);
	for (i = 0; i < len; i++) {
		pos += sprintf(str + pos, "[%02X] ", buf[i] & 0xff);
	}
	test_info_append(str);
	return;
}

static void test_info_clear() {
	SendMessage(GetDlgItem(g_hwnd, IDC_INFO), EM_SETSEL, -1, 0);
	SendMessageA(GetDlgItem(g_hwnd, IDC_INFO), WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");

}

static void test_select_all() {
	int i = 0;
	int cnt = sizeof(g_tos) / sizeof(g_tos[0]);
	int flag = 0;

	for (i = 0; i < cnt; i++) {
		if (g_tos[i].id == CHK_LAN) {
			flag = 1;
		}

		g_tos[i].checked = 1;

		if (flag) {
			CheckDlgButton(g_hwnd, g_tos[i].id, BST_CHECKED);
		}
	}

}
