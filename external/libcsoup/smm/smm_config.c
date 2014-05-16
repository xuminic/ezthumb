
/*  smm_config.c - configure parameters management 

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of LIBSMM, System Masquerade Module library

    LIBSMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIBSMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"

#ifdef	CFG_WIN32_API
static HKEY RegOpenMainKey(HKEY hRootKey, char *mkey, int mode);
static HKEY RegCreatePath(int sysroot, char *path);
static int RegReadString(HKEY hMainKey, char *skey, char *buf, int blen);
static int RegReadLong(HKEY hMainKey, char *skey, long *val);
static int RegWriteString(HKEY hMainKey, char *skey, char *value);
static int RegWriteLong(HKEY hMainKey, char *skey, long val);
static BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey, int klen);

void *smm_config_open(int sysroot, int mode, char *path, char *fname)
{
	HKEY	hRootKey, hPathKey;

	if ((hPathKey = RegCreatePath(sysroot, path)) == NULL) {
		return NULL;
	}
	hRootKey = RegOpenMainKey(hPathKey, fname, mode);
	RegCloseKey(hPathKey);
	return hRootKey;
}

int smm_config_flush(void *cfg)
{
	if (RegFlushKey((HKEY) cfg) == ERROR_SUCCESS) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	return smm_errno_update(SMM_ERR_NULL);
}

int smm_config_close(void *cfg)
{
	RegFlushKey((HKEY) cfg);
	if (RegCloseKey((HKEY) cfg) == ERROR_SUCCESS) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	return smm_errno_update(SMM_ERR_NULL);
}

int smm_config_delete(int sysroot, char *path, char *fname)
{
	HKEY	hPathKey;
	TCHAR	*wkey;
	LONG	rcode;

	if (fname == NULL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	if ((hPathKey = RegCreatePath(sysroot, path)) == NULL) {
		return smm_errno_update(SMM_ERR_ACCESS);
	}

	/* fabricate the key name */
	if ((wkey = smm_alloc(MAX_PATH * 2 * sizeof(TCHAR))) == NULL) {
		RegCloseKey(hPathKey);
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	MultiByteToWideChar(smm_codepage(), 0, fname, -1, wkey, MAX_PATH - 1);

	rcode = RegDelnodeRecurse(hPathKey, wkey, MAX_PATH * 2);

	RegCloseKey(hPathKey);
	smm_free(wkey);
       
	if (rcode == TRUE) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	return smm_errno_update(SMM_ERR_ACCESS);
}

char *smm_config_read_alloc(void *cfg, char *mkey, char *skey)
{
	HKEY	hMainKey;
	char	*buf;
	int	blen;

	if (skey == NULL) {
		smm_errno_update(SMM_ERR_NULL);
		return NULL;
	}
	hMainKey = RegOpenMainKey(cfg, mkey, SMM_CFGMODE_RDONLY);
	if (hMainKey == NULL) {
		smm_errno_update(SMM_ERR_ACCESS);
		return NULL;
	}

	buf = NULL;
	if ((blen = RegReadString(hMainKey, skey, NULL, 0)) > 0) {
		blen += 2;
		buf = smm_alloc(blen);
		RegReadString(hMainKey, skey, buf, blen);
	}

	if (hMainKey != cfg) {
		RegCloseKey(hMainKey);
	}
	return buf;
}

int smm_config_write(void *cfg, char *mkey, char *skey, char *value)
{
	HKEY	hMainKey;
	int	rc;

	if (!skey || !value) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	if ((hMainKey = RegOpenMainKey(cfg, mkey, SMM_CFGMODE_RWC)) == NULL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	rc = RegWriteString(hMainKey, skey, value);

	if (hMainKey != cfg) {
		RegCloseKey(hMainKey);
	}
	return rc;
}

int smm_config_read_long(void *cfg, char *mkey, char *skey, long *val)
{
	HKEY	hMainKey;
	int	rc;

	if (skey == NULL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	hMainKey = RegOpenMainKey(cfg, mkey, SMM_CFGMODE_RDONLY);
	if (hMainKey == NULL) {
		return smm_errno_update(SMM_ERR_ACCESS);
	}
	rc = RegReadLong(hMainKey, skey, val);

	if (hMainKey != cfg) {
		RegCloseKey(hMainKey);
	}
	return rc;
}

int smm_config_write_long(void *cfg, char *mkey, char *skey, long val)
{
	HKEY	hMainKey;
	int	rc;

	if (skey == NULL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	if ((hMainKey = RegOpenMainKey(cfg, mkey, SMM_CFGMODE_RWC)) == NULL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	rc = RegWriteLong(hMainKey, skey, val);

	if (hMainKey != cfg) {
		RegCloseKey(hMainKey);
	}
	return rc;
}

static HKEY RegOpenMainKey(HKEY hRootKey, char *mkey, int mode)
{
	HKEY	hMainKey;
	TCHAR	*wkey;
	LONG	rc;

	if (mkey == NULL) {
		return hRootKey;
	}
	if ((wkey = smm_mbstowcs_alloc(mkey)) == NULL) {
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}

	switch (mode) {
	case SMM_CFGMODE_RDONLY:
		rc = RegOpenKeyEx(hRootKey, wkey, 0, KEY_READ, &hMainKey);
		break;
	case SMM_CFGMODE_RWC:
		rc = RegCreateKeyEx(hRootKey, wkey, 0, NULL, 0, 
				KEY_ALL_ACCESS, NULL, &hMainKey, NULL);
		break;
	default:	/* SMM_CFGMODE_RDWR */
		rc = RegOpenKeyEx(hRootKey, wkey, 0, KEY_ALL_ACCESS, &hMainKey);
		break;
	}

	smm_free(wkey);			
	if (rc == ERROR_SUCCESS) {
		smm_errno_update(SMM_ERR_NONE);
		return hMainKey;
	}
	smm_errno_update(SMM_ERR_ACCESS);
	return NULL;
}

static HKEY RegCreatePath(int sysroot, char *path)
{
	HKEY	hPathKey, hSysKey;
	LONG	rc;
	TCHAR	*wkey;
	char	*pkey;
	int	extra;

	extra = 4;
	if (path) {
		extra += strlen(path);
	}
	switch (sysroot) {
	case SMM_CFGROOT_USER:
		hSysKey = HKEY_CURRENT_USER;
		pkey = csc_strcpy_alloc("CONSOLE\\", extra);
		break;
	case SMM_CFGROOT_SYSTEM:
		hSysKey = HKEY_LOCAL_MACHINE;
		pkey = csc_strcpy_alloc("SOFTWARE\\", extra);
		break;
	case SMM_CFGROOT_CURRENT:
		/* don't do anything */
		smm_errno_update(SMM_ERR_NONE);
		return NULL;
	default:	/* SMM_CFGROOT_DESKTOP */
		hSysKey = HKEY_CURRENT_USER;
		pkey = csc_strcpy_alloc("SOFTWARE\\", extra);
		break;
	}
	if (pkey == NULL) {
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}
	if (path) {
		strcat(pkey, path);
	}

	if ((wkey = smm_mbstowcs_alloc(pkey)) == NULL) {
		smm_free(pkey);
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}

	/* The good thing of RegCreateKeyEx() is that it can create a string
	 * of subkeys without creating one by one. For example: A\\B\\C */
	rc = RegCreateKeyEx(hSysKey, wkey, 0, NULL, 0,
			KEY_ALL_ACCESS, NULL, &hPathKey, NULL);

	smm_free(wkey);
	smm_free(pkey);

	if (rc == ERROR_SUCCESS) { 
		return hPathKey;
	}
	return NULL;
}

static int RegReadString(HKEY hMainKey, char *skey, char *buf, int blen)
{
	TCHAR	*wkey, *wval;
	DWORD	slen;
	int	vlen;

	if ((wkey = smm_mbstowcs_alloc(skey)) == NULL) {
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	if (RegQueryValueEx(hMainKey, wkey, NULL, NULL, NULL, &slen)
			!= ERROR_SUCCESS) {
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_ACCESS);
	}
	slen += 2;
	if ((wval = smm_alloc(slen)) == NULL) {
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	if (RegQueryValueEx(hMainKey, wkey, NULL, NULL, (BYTE*) wval, &slen)
			!= ERROR_SUCCESS) {
		smm_free(wval);
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_ACCESS);
	}

	/* see smm_wcstombs.c for details */
	vlen = WideCharToMultiByte(smm_codepage(), 
			0, wval, -1, NULL, 0, NULL, NULL);
	if (vlen <= 0) {
		smm_free(wval);
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_LENGTH);
	}
	if (buf && (blen > vlen)) {
		WideCharToMultiByte(smm_codepage(), 
				0, wval, -1, buf, blen, NULL, NULL);
	}
	smm_free(wval);
	smm_free(wkey);
	return vlen;
}

static int RegReadLong(HKEY hMainKey, char *skey, long *val)
{
	DWORD	vlen;
	TCHAR	*wkey;

	if ((wkey = smm_mbstowcs_alloc(skey)) == NULL) {
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	vlen = sizeof(long);
	if (RegQueryValueEx(hMainKey, wkey, NULL, NULL, (BYTE*) val, &vlen)
			== ERROR_SUCCESS) {
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_NONE);
	}
	smm_free(wkey);
	return smm_errno_update(SMM_ERR_ACCESS);
}

static int RegWriteString(HKEY hMainKey, char *skey, char *value)
{
	TCHAR	*wkey, *wval;
	LONG	rc;

	if ((wkey = smm_mbstowcs_alloc(skey)) == NULL) {
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	if ((wval = smm_mbstowcs_alloc(value)) == NULL) {
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_LOWMEM);
	}

	rc = RegSetValueEx(hMainKey, wkey, 0, REG_SZ, (const BYTE *) wval, 
			(lstrlen(wval)+1) * sizeof(TCHAR));

	smm_free(wval);
	smm_free(wkey);

	if (rc == ERROR_SUCCESS) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	return smm_errno_update(SMM_ERR_ACCESS);
}

static int RegWriteLong(HKEY hMainKey, char *skey, long val)
{
	TCHAR	*wkey;

	if ((wkey = smm_mbstowcs_alloc(skey)) == NULL) {
		return smm_errno_update(SMM_ERR_LOWMEM);
	}

	if (RegSetValueEx(hMainKey, wkey, 0, REG_DWORD, (BYTE *) &val, 
				sizeof(long)) == ERROR_SUCCESS) {
		smm_free(wkey);
		return smm_errno_update(SMM_ERR_NONE);
	}
	smm_free(wkey);
	return smm_errno_update(SMM_ERR_ACCESS);
}


/* This code was picked form MSDN, a little modified */
static BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey, int buflen)
{
	LPTSTR	lpEnd;
	LONG	lResult;
	DWORD	dwSize;
	TCHAR	szName[MAX_PATH];
	HKEY	hKey;

	/* First, see if we can delete the key without having to recurse. */
	if (RegDeleteKey(hKeyRoot, lpSubKey) == ERROR_SUCCESS) {
		return TRUE;
	}

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS) {
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return TRUE;	/* not guilty */
		} else {
			return FALSE;	/* access denied */
		}
	}

	/* Check for an ending slash and add one if it is missing. */
	if (lstrlen(lpSubKey) >= buflen) {
		return FALSE;	/* low buffer memory */
	}
	lpEnd = lpSubKey + lstrlen(lpSubKey);
	if (*(lpEnd - 1) != TEXT('\\')) {
		*lpEnd++ = TEXT('\\');
		*lpEnd = TEXT('\0');
	}

	/* Enumerate the keys */
	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, 
			NULL, NULL, NULL);
	if (lResult == ERROR_SUCCESS) {
		do {
			if (lstrlen(lpSubKey) + lstrlen(szName) >= buflen) {
				break;	/* path is too long */
			}
			lstrcpy(lpEnd, szName);
			if (!RegDelnodeRecurse(hKeyRoot, lpSubKey, buflen)) {
				break;
			}
			dwSize = MAX_PATH;
			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, 
					NULL, NULL, NULL, NULL);
		} while (lResult == ERROR_SUCCESS);
	}
	lpEnd--;
	*lpEnd = TEXT('\0');
	RegCloseKey (hKey);

	/* Try again to delete the key. */
	if (RegDeleteKey(hKeyRoot, lpSubKey) == ERROR_SUCCESS) {
		return TRUE;
	}
	return FALSE;
}
#endif

#ifdef	CFG_UNIX_API
#include <unistd.h>
#include <errno.h>

static char *smm_config_mkpath(int sysroot, char *path, int extra);

void *smm_config_open(int sysroot, int mode, char *path, char *fname)
{
	void	*root;

	if ((path = smm_config_mkpath(sysroot, path, 0)) == NULL) {
		return NULL;
	}
	root = csc_cfg_open(path, fname, mode);
	smm_free(path);
	return root;
}

int smm_config_flush(void *cfg)
{
	return csc_cfg_flush(cfg);
}

int smm_config_close(void *cfg)
{
	return csc_cfg_close(cfg);
}

int smm_config_delete(int sysroot, char *path, char *fname)
{
	path = smm_config_mkpath(sysroot, path, strlen(fname) + 4);
	if (path == NULL) {
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	strcat(path, "/");
	strcat(path, fname);
	
	if (unlink(path) == 0) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	if (errno == EACCES) {
		return smm_errno_update(SMM_ERR_ACCESS);
	}
	return smm_errno_update(SMM_ERR_NULL);
}

char *smm_config_read_alloc(void *cfg, char *mkey, char *skey)
{
	return csc_cfg_copy(cfg, mkey, skey, 4);
}

int smm_config_write(void *cfg, char *mkey, char *skey, char *value)
{
	return csc_cfg_write(cfg, mkey, skey, value);
}

int smm_config_read_long(void *cfg, char *mkey, char *skey, long *val)
{
	return csc_cfg_read_long(cfg, mkey, skey, val);
}

int smm_config_write_long(void *cfg, char *mkey, char *skey, long val)
{
	return csc_cfg_write_longlong(cfg, mkey, skey, (long long) val);
}

static char *smm_config_mkpath(int sysroot, char *path, int extra)
{
	char	*fullpath;

	extra += 4;
	if (path) {
		extra += strlen(path);
	}
	switch (sysroot) {
	case SMM_CFGROOT_USER:
		fullpath = csc_strcpy_alloc(getenv("HOME"), extra);
		break;
	case SMM_CFGROOT_SYSTEM:
		fullpath = csc_strcpy_alloc("/etc", extra);
		break;
	case SMM_CFGROOT_CURRENT:
		fullpath = csc_strcpy_alloc(".", extra);
		break;
	default:	/* SMM_CFGROOT_DESKTOP */
		fullpath = csc_strcpy_alloc(getenv("HOME"), extra + 16);
		if (fullpath) {
			strcat(fullpath, "/.config");
			if (path) {
				strcat(fullpath, "/");
				strcat(fullpath, path);
			}
		}
		break;
	}
	if (fullpath == NULL) {
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}
	return fullpath;
}
#endif

int smm_config_read_int(void *cfg, char *mkey, char *skey, int *val)
{
	long	ival;
	int	rc;

	rc = smm_config_read_long(cfg, mkey, skey, &ival);
	if (val && (rc == SMM_ERR_NONE)) {
		*val = (int) ival;
	}
	return rc;
}

int smm_config_write_int(void *cfg, char *mkey, char *skey, int val)
{
	return smm_config_write_long(cfg, mkey, skey, (long) val);
}

