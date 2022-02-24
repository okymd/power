/**
	@file
	power - a power object
	jeremy bernstein - jeremy@bootsquad.com

	@ingroup	examples
*/

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object

#include "Windows.h"

////////////////////////// object struct
typedef struct _power
{
	t_object	ob;
	t_atom		val;
	t_symbol	*name;
	void		*out;
	void		*out2;
} t_power;

///////////////////////// function prototypes
//// standard set
void *power_new(t_symbol *s, long argc, t_atom *argv);
void power_free(t_power *x);
void power_assist(t_power *x, void *b, long m, long a, char *s);

void power_bang(t_power *x);
void power_dblclick(t_power *x);
void power_shutdown(t_power* x, t_symbol* s,long argc, t_atom* argv);
void power_doshutdown(t_power* x, t_symbol* s, long argc, t_atom* argv);
void power_abort(t_power* x, t_symbol* s);
void power_doabort(t_power* x, t_symbol* s, long argc, t_atom* argv);
void power_reboot(t_power* x, t_symbol* s, long argc, t_atom* argv);
void power_doreboot(t_power* x, t_symbol* s, long argc, t_atom* argv);


//////////////////////// global class pointer variable
void *power_class;

SYSTEM_POWER_STATUS curStatus;

void ext_main(void *r)
{
	t_class *c;

	c = class_new("power", (method)power_new, (method)power_free, (long)sizeof(t_power),
				  0L /* leave NULL!! */, A_GIMME, 0);

	class_addmethod(c, (method)power_bang,			"bang", 0);
	class_addmethod(c, (method)power_shutdown,		"shutdown", A_GIMME,0);
	class_addmethod(c, (method)power_reboot,		"reboot",   A_GIMME,0);
 	class_addmethod(c, (method)power_abort,			"abort",	 0);


	/* you CAN'T call this from the patcher */
	class_addmethod(c, (method)power_assist,			"assist",		A_CANT, 0);
	class_addmethod(c, (method)power_dblclick,			"dblclick",		A_CANT, 0);

	//CLASS_ATTR_SYM(c, "name", 0, t_power, name);

	class_register(CLASS_BOX, c);
	power_class = c;
}

void power_assist(t_power *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
		switch (a) {
		case 0:	sprintf(s, "bang output status,shutdown,reboot,abort"); break;
		}
	}
	else {	// outlet
		switch (a) {
		case 0:	sprintf(s, "AC condition"); break;
		case 1: sprintf(s, "Battery Percent"); break;
		}
	}
}

void power_free(t_power *x)
{

	;
}

void power_dblclick(t_power *x)
{
	object_post((t_object *)x, "I got a double-click");
}


void power_bang(t_power *x)
{
	GetSystemPowerStatus(&curStatus);
	outlet_int(x->out,	curStatus.BatteryLifePercent);
	outlet_int(x->out2,	curStatus.ACLineStatus);
}

void *power_new(t_symbol *s, long argc, t_atom *argv)
{
	t_power* x = object_alloc(power_class);
	x->out =  intout(x);
	x->out2 = intout(x);
	return (x);
}


void power_shutdown(t_power* x, t_symbol* s,long argc , t_atom* argv)
{
	defer((t_object*)x, (method)power_doshutdown, s, argc, argv);
}

void power_doshutdown(t_power* x, t_symbol* s, long argc, t_atom* argv)
{

	if (!EnablePrivileges(SE_SHUTDOWN_NAME, TRUE)) {
		object_post((t_object*)x, "EnablePrivileges failed");
		return ;
	}
	int timeout = 10;

	t_atom* ap;
	if (argc >= 1) {
		ap = argv;
		if (atom_gettype(ap) == A_LONG) {
			timeout=atom_getlong(ap);
			if (timeout < 0)timeout = 0;
		}
	}

	char msg[128];
	sprintf(msg, "Shutdown in %d seconds", timeout);
	object_post((t_object*)x, msg);
	InitiateSystemShutdownEx(NULL,msg, timeout, TRUE, FALSE, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_FLAG_PLANNED);
}
void power_reboot(t_power* x, t_symbol* s,long argc,t_atom* argv)
{
	defer((t_object*)x, (method)power_doreboot, s, argc, argv);
}

void power_doreboot(t_power* x, t_symbol* s, long argc, t_atom* argv)
{

	if (!EnablePrivileges(SE_SHUTDOWN_NAME, TRUE)) {
		object_post((t_object*)x, "EnablePrivileges failed");
		return;
	}
	int timeout = 10;

	t_atom* ap;
	if (argc >= 1) {
		ap = argv;
		if (atom_gettype(ap) == A_LONG) {
			timeout = (int)atom_getlong(ap);
			if (timeout < 0)timeout = 0;
		}
	}

	char msg[128];
	sprintf(msg, "Reboot in %d seconds", timeout);
	object_post((t_object*)x, msg);
	InitiateSystemShutdownEx(NULL, msg, timeout, TRUE, TRUE, SHTDN_REASON_MAJOR_APPLICATION |  SHTDN_REASON_FLAG_PLANNED);//enalbe reboot
}
void power_abort(t_power* x, t_symbol* s)
{
	defer((t_object*)x, (method)power_doabort, s, 0, NULL);
}
BOOL EnablePrivileges(LPTSTR lpPrivilegeName, BOOL bEnable)
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tokenPrivileges;
	BOOL bRet;

	//1.OpenProcessToken関数で、プロセストークンを取得する
	bRet = OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hToken);
	if (!bRet) {
		return FALSE;
	}

	//2.LookupPrivilegeValue関数で、特権に対応するLUID(ローカル一意識別子)を取得する
	bRet = LookupPrivilegeValue(NULL, lpPrivilegeName, &luid);
	if (bRet) {

		//3.TOKEN_PRIVILEGES型のオブジェクトに、LUID(ローカル一意識別子)と特権の属性(有効にするか無効にするか)を指定する
		tokenPrivileges.PrivilegeCount = 1;
		tokenPrivileges.Privileges[0].Luid = luid;
		tokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

		//4.AdjustTokenPrivileges関数で、特権を有効にする
		AdjustTokenPrivileges(hToken,
			FALSE,
			&tokenPrivileges, 0, 0, 0);

		bRet = GetLastError() == ERROR_SUCCESS;

	}

	CloseHandle(hToken);

	return bRet;
}

void power_doabort(t_power* x, t_symbol* s, long argc, t_atom* argv)
{
	object_post((t_object*)x, "Abort Shutdown");
	AbortSystemShutdown(NULL);
}