/*  Dummy IShellFolder namespace extension.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "stdafx.h"

#include "DummyFolder.h"
#include "Pidl.h"

CDummyFolder::CDummyFolder()
{
	::ZeroMemory(&m_apidl[0], sizeof m_apidl[0]);
}

CDummyFolder::~CDummyFolder()
{
	if (m_apidl[0])
		::ILFree(m_apidl[0]);
}

HRESULT CDummyFolder::FinalConstruct()
{
	return S_OK;
}

void CDummyFolder::FinalRelease()
{
}

STDMETHODIMP CDummyFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
	HRESULT hr = __super::Initialize(pidl);

	if (SUCCEEDED(hr))
	{
		int level;
		try
		{
			// If the last item is of our type, interpret it in order to
			// get next level, otherwise default to a level of 0
			PCUITEMID_CHILD pidlEnd = ::ILFindLastID(pidl);

			ValidatePidl(pidlEnd);

			level = (reinterpret_cast<const DummyItemId *>(pidlEnd)->level) + 1;
		}
		catch (...)
		{
			level = 0;
		}

		if (m_apidl[0])
			::ILFree(m_apidl[0]);

		size_t nDummyPIDLSize = sizeof DummyItemId + sizeof USHORT;
		m_apidl[0] = (PITEMID_CHILD)CoTaskMemAlloc(nDummyPIDLSize);
		::ZeroMemory(m_apidl[0], nDummyPIDLSize);

		// Use first PIDL member as a DummyItemId structure
		DummyItemId *pidlDummy = (DummyItemId *)m_apidl[0];

		// Fill members of the PIDL with data
		pidlDummy->cb = sizeof DummyItemId;
		pidlDummy->dwFingerprint = DummyItemId::FINGERPRINT; // Sign with fingerprint
		pidlDummy->level = level;
	}

	return hr;
}

void CDummyFolder::ValidatePidl(PCUIDLIST_RELATIVE pidl) const throw(...)
{
	if (pidl == NULL)
		AtlThrow(E_POINTER);

	const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);

	if (pitemid->cb != sizeof DummyItemId ||
		pitemid->dwFingerprint != DummyItemId::FINGERPRINT)
		AtlThrow(E_INVALIDARG);
}

CLSID CDummyFolder::GetCLSID() const
{
	return __uuidof(this);
}

STDMETHODIMP CDummyFolder::ParseDisplayName(
	HWND hwnd, IBindCtx *pbc, PWSTR pwszDisplayName, ULONG *pchEaten, 
	PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pwszDisplayName, E_POINTER);
	ATLENSURE_RETURN_HR(*pwszDisplayName != L'\0', E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	if (pchEaten)
		*pchEaten = 0;
	*ppidl = NULL;
	if (pdwAttributes)
		*pdwAttributes = 0;

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CDummyFolder::EnumObjects(
	HWND hwnd, SHCONTF grfFlags, IEnumIDList **ppenumIDList)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(ppenumIDList, E_POINTER);

	*ppenumIDList = NULL;

	typedef CComEnum<IEnumIDList, &__uuidof(IEnumIDList), PITEMID_CHILD, _CopyPidl>
	        CComEnumIDList;

	CComObject<CComEnumIDList> *pEnum = NULL;
	HRESULT hr = pEnum->CreateInstance(&pEnum);
	if (SUCCEEDED(hr))
	{
		pEnum->AddRef();

		hr = pEnum->Init(&m_apidl[0], &m_apidl[1], GetUnknown());
		ATLASSERT(SUCCEEDED(hr));

		hr = pEnum->QueryInterface(ppenumIDList);
		ATLASSERT(SUCCEEDED(hr));

		pEnum->Release();
	}

	return hr;
}

IShellFolder* CDummyFolder::CreateSubfolder(PCIDLIST_ABSOLUTE pidlRoot) const
throw(...)
{
	HRESULT hr;

	// Create and initialise new folder object for subfolder
	CComPtr<CDummyFolder> spDummyFolder = spDummyFolder->Create();
	hr = spDummyFolder->Initialize(pidlRoot);
	ATLENSURE_THROW(SUCCEEDED(hr), hr);

	CComQIPtr<IShellFolder> spFolder = spDummyFolder;
	ATLENSURE_THROW(spFolder, E_NOINTERFACE);

	return spFolder.Detach();
}

/**
 * Determine the relative order of two file objects or folders.
 *
 * @implementing IShellFolder
 *
 * Given their item identifier lists, compare the two objects and return a value
 * in the HRESULT indicating the result of the comparison:
 * - Negative: pidl1 < pidl2
 * - Positive: pidl1 > pidl2
 * - Zero:     pidl1 == pidl2
 */
STDMETHODIMP CDummyFolder::CompareIDs( 
		LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl1, E_POINTER);
	ATLENSURE_RETURN_HR(pidl2, E_POINTER);

	USHORT uColumn = LOWORD(lParam);
	bool fCompareAllFields = (HIWORD(lParam) == SHCIDS_ALLFIELDS);
	bool fCanonical = (HIWORD(lParam) == SHCIDS_CANONICALONLY);
	ATLASSERT(!fCompareAllFields || uColumn == 0);
	ATLASSERT(!fCanonical || !fCompareAllFields);

	// TODO: Must recurse down PIDL, making the comparison at each stage.
	if (ILIsChild(pidl1) && ILIsChild(pidl2))
	{
		const DummyItemId *pitemid1 = reinterpret_cast<const DummyItemId *>(pidl1);
		const DummyItemId *pitemid2 = reinterpret_cast<const DummyItemId *>(pidl2);
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, pitemid1->level - pitemid2->level);
	}
	else
	{
		return MAKE_HRESULT(
			SEVERITY_SUCCESS, 0, ::ILGetSize(pidl1) - ::ILGetSize(pidl2));
	}
}

/**
 * Create one of the objects associated with the *current folder* view.
 *
 * @implementing IShellFolder
 *
 * The types of object which can be requested include IShellView, IContextMenu, 
 * IExtractIcon, IQueryInfo, IShellDetails or IDropTarget.  This method is in
 * contrast to GetUIObjectOf() which performs the same task but for an item
 * contained *within* the current folder rather than the folder itself.
 */
STDMETHODIMP CDummyFolder::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(ppv, E_POINTER);

	*ppv = NULL;

	try
	{
		if (riid == __uuidof(IShellView))
		{
			// Create a pointer to this IShellFolder to pass to view
			SFV_CREATE sfvData = { sizeof sfvData, 0 };
			CComPtr<IShellFolder> spFolder = this;
			ATLENSURE_THROW(spFolder, E_NOINTERFACE);
			sfvData.pshf = spFolder;
			sfvData.psfvcb = NULL;
			sfvData.psvOuter = NULL;

			// Create Default Shell Folder View object (aka DEFVIEW)
			return SHCreateShellFolderView(&sfvData, (IShellView**)ppv);
		}
		else
			return E_NOINTERFACE;
	}
	catchCom()
}

STDMETHODIMP CDummyFolder::GetAttributesOf( 
	UINT cpidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(apidl, E_POINTER);
	ATLENSURE_RETURN_HR(rgfInOut, E_POINTER);

	SFGAOF rgfRequested = *rgfInOut;
	*rgfInOut = 0;

	if (rgfRequested & SFGAO_FOLDER)
		*rgfInOut |= SFGAO_FOLDER;
	if (rgfRequested & SFGAO_HASSUBFOLDER)
		*rgfInOut |= SFGAO_HASSUBFOLDER;
	if (rgfRequested & SFGAO_FILESYSANCESTOR)
		*rgfInOut |= SFGAO_FILESYSANCESTOR;
	if (rgfRequested & SFGAO_BROWSABLE)
		*rgfInOut |= SFGAO_BROWSABLE;

	return S_OK;
}

STDMETHODIMP CDummyFolder::GetUIObjectOf(
	HWND hwndOwner, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
	UINT *rgfReserved, void **ppv)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(apidl, E_POINTER);
	ATLENSURE_RETURN_HR(ppv, E_POINTER);

	*ppv = NULL;

	HRESULT hr;

	if (riid == __uuidof(IQueryAssociations))
	{
		ATLTRACE("\t\tRequest: IQueryAssociations\n");
		ATLASSERT(cpidl == 1);

		CComPtr<IQueryAssociations> spAssoc;
		hr = ::AssocCreate(CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		
		// Initialise default assoc provider for Folders
		hr = spAssoc->Init(0, L"Folder", NULL, NULL);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		*ppv = spAssoc.Detach();
	}
	else if (riid == __uuidof(IContextMenu) || riid == __uuidof(IContextMenu2) ||
		     riid == __uuidof(IContextMenu3))
	{
		ATLTRACE("\t\tRequest: IContextMenu\n");

		CComPtr<IShellFolder> spThisFolder = this;
		ATLENSURE_RETURN_HR(spThisFolder, E_OUTOFMEMORY);

		CComPtr<IContextMenu> spMenu;
		hr = ::CDefFolderMenu_Create2(
			m_pidlRoot, hwndOwner, cpidl, apidl, spThisFolder, MenuCallback, 0, NULL, 
			&spMenu);
		ATLASSERT(SUCCEEDED(hr));

		hr = spMenu->QueryInterface(riid, ppv);
	}
	else
	{
		ATLTRACE("\t\tRequest: <unknown>\n");
		hr = E_NOINTERFACE;
	}

	return hr;
}

STDMETHODIMP CDummyFolder::GetDisplayNameOf( 
	PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pName)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(pName, E_POINTER);

	::ZeroMemory(pName, sizeof *pName);

	const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);

	CString strName;
	strName.AppendFormat(L"Level %d", pitemid->level);
	
	// Store in a STRRET and return
	pName->uType = STRRET_WSTR;
	return ::SHStrDup(strName, &(pName->pOleStr));
}

STDMETHODIMP CDummyFolder::SetNameOf( 
	HWND hwnd, PCUITEMID_CHILD pidl, PCWSTR pwszName, SHGDNF uFlags, 
	PITEMID_CHILD *ppidlOut)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(pwszName, E_POINTER);
	ATLENSURE_RETURN_HR(*pwszName != L'\0', E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppidlOut, E_POINTER);

	*ppidlOut = NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CDummyFolder::GetDefaultColumn(
	DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pSort, E_POINTER);
	ATLENSURE_RETURN_HR(pDisplay, E_POINTER);

	*pSort = 0;
	*pDisplay = 0;

	return S_OK;
}

STDMETHODIMP CDummyFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pcsFlags, E_POINTER);

	*pcsFlags = 0;

	if (iColumn != 0)
		return E_FAIL;

	*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

	return S_OK;
}

STDMETHODIMP CDummyFolder::GetDetailsOf(
	PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(psd, E_POINTER);

	::ZeroMemory(psd, sizeof SHELLDETAILS);

	if (iColumn != 0)
		return E_FAIL;

	CString str;

	if (pidl == NULL) // Wants header
	{
		str = L"Name";
	}
	else              // Wants contents
	{
		const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);
		str.AppendFormat(L"Level %d", pitemid->level);
	}
	
	psd->fmt = LVCFMT_LEFT;
	psd->cxChar = 4;

	// Store in STRRET and return
	psd->str.uType = STRRET_WSTR;
	return ::SHStrDup(str, &(psd->str.pOleStr));
}

STDMETHODIMP CDummyFolder::GetDetailsEx(
	PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(pscid, E_POINTER);
	ATLENSURE_RETURN_HR(pv, E_POINTER);

	//::VariantClear(pv);

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CDummyFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pscid, E_POINTER);

	::ZeroMemory(pscid, sizeof *pscid);

	ATLTRACENOTIMPL(__FUNCTION__);
}



/**
 * Cracks open the @c DFM_* callback messages and dispatched them to handlers.
 */
HRESULT CDummyFolder::OnMenuCallback(
	HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	ATLTRACE(__FUNCTION__" called (uMsg=%d)\n", uMsg);
	UNREFERENCED_PARAMETER(hwnd);

	switch (uMsg)
	{
	case DFM_MERGECONTEXTMENU:
		return this->OnMergeContextMenu(
			hwnd,
			pdtobj,
			static_cast<UINT>(wParam),
			*reinterpret_cast<QCMINFO *>(lParam)
		);
	case DFM_INVOKECOMMAND:
		return this->OnInvokeCommand(
			hwnd,
			pdtobj,
			static_cast<int>(wParam),
			reinterpret_cast<PCWSTR>(lParam)
		);
	case DFM_INVOKECOMMANDEX:
		return this->OnInvokeCommandEx(
			hwnd,
			pdtobj,
			static_cast<int>(wParam),
			reinterpret_cast<PDFMICS>(lParam)
		);
	default:
		return E_NOTIMPL;
	}
}

/**
 * Handle @c DFM_MERGECONTEXTMENU callback.
 */
HRESULT CDummyFolder::OnMergeContextMenu(
	HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info )
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(pDataObj);
	UNREFERENCED_PARAMETER(uFlags);
	UNREFERENCED_PARAMETER(info);

	// It seems we have to return S_OK even if we do nothing else or Explorer
	// won't put Open as the default item and in the right order
	return S_OK;
}

/**
 * Handle @c DFM_INVOKECOMMAND callback.
 */
HRESULT CDummyFolder::OnInvokeCommand(
	HWND hwnd, IDataObject *pDataObj, int idCmd, PCWSTR pszArgs )
{
	ATLTRACE(__FUNCTION__" called (hwnd=%p, pDataObj=%p, idCmd=%d, "
		"pszArgs=%ls)\n", hwnd, pDataObj, idCmd, pszArgs);

	return S_FALSE;
}

/**
 * Handle @c DFM_INVOKECOMMANDEX callback.
 */
HRESULT CDummyFolder::OnInvokeCommandEx(
	HWND hwnd, IDataObject *pDataObj, int idCmd, PDFMICS pdfmics )
{
	ATLTRACE(__FUNCTION__" called (pDataObj=%p, idCmd=%d, pdfmics=%p)\n",
		pDataObj, idCmd, pdfmics);

	switch (idCmd)
	{
	default:
		return S_FALSE;
	}
}
