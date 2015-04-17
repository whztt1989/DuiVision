#include "StdAfx.h"
#include "Panel.h"

#define	SCROLL_V	1	// 滚动条控件ID

CDuiPanel::CDuiPanel(HWND hWnd, CDuiObject* pDuiObject)
			: CControlBaseFont(hWnd, pDuiObject)
{
	m_bEnableScroll = TRUE;
	m_nScrollWidth = 8;

	CRect rcScroll = CRect(0,0,0,0);
	rcScroll.top;
	rcScroll.left = rcScroll.right - m_nScrollWidth;

 	CControlBase * pControlBase = NULL;
 	pControlBase = new CScrollV(hWnd, this, SCROLL_V, rcScroll);
 	m_vecControl.push_back(pControlBase);
	m_pControScrollV = (CControlBaseFont*)pControlBase;

	m_strXmlFile = _T("");
	m_nVirtualHeight = 0;
	m_nVirtualTop = 0;

	m_hPluginHandle = NULL;
	m_strPluginFile = _T("");
	m_pDuiPluginObject = NULL;

	m_bDblClk = true;

	m_bInit = false;
}

CDuiPanel::~CDuiPanel(void)
{
	// 释放界面插件对象和动态库
	if(m_pDuiPluginObject != NULL)
	{
		m_pDuiPluginObject->Release();
		m_pDuiPluginObject = NULL;
	}

	if(m_hPluginHandle != NULL)
	{
		FreeLibrary(m_hPluginHandle);
		m_hPluginHandle = NULL;
	}
}

// 根据控件名创建控件实例
CControlBase* CDuiPanel::_CreateControlByName(LPCTSTR lpszName)
{
	// 如果已经设置了窗口句柄,使用窗口句柄创建控件
	HWND hWnd = m_hWnd;
	if(hWnd != NULL)
	{
		return DuiSystem::CreateControlByName(lpszName, hWnd, this);
	}

	// 查找父对话框的窗口句柄,通过父对话框句柄创建控件
	CDuiObject* pParentObj = GetParent();
	while((pParentObj != NULL) && (!pParentObj->IsClass(_T("dlg"))) && (!pParentObj->IsClass(_T("popup"))))
	{
		pParentObj = ((CControlBase*)pParentObj)->GetParent();
	}

	if((pParentObj != NULL) && pParentObj->IsClass(_T("dlg")))
	{
		return DuiSystem::CreateControlByName(lpszName, ((CDlgBase*)pParentObj)->GetSafeHwnd(), this);
	}else
	if((pParentObj != NULL) && pParentObj->IsClass(_T("popup")))
	{
		return DuiSystem::CreateControlByName(lpszName, ((CDlgPopup*)pParentObj)->GetSafeHwnd(), this);
	}

	return NULL;
}

// 加载XML节点，解析节点中的属性信息设置到当前控件的属性中
BOOL CDuiPanel::Load(DuiXmlNode pXmlElem, BOOL bLoadSubControl)
{
	if(!__super::Load(pXmlElem))
	{
		return FALSE;
	}

    // 使用XML节点初始化div控件
	if(pXmlElem != NULL)
	{
		InitUI(m_rc, pXmlElem);
	}

    return TRUE;
}

// 加载XML文件
BOOL CDuiPanel::LoadXmlFile(CString strFileName)
{
	DuiXmlDocument xmlDoc;
	DuiXmlNode pDivElem;

	if(!DuiSystem::Instance()->LoadXmlFile(xmlDoc, strFileName))
	{
		DuiSystem::LogEvent(LOG_LEVEL_ERROR, L"CDuiPanel::LoadXmlFile %s failed", strFileName);
		return FALSE;
	}

	m_strXmlFile = strFileName;
	pDivElem = xmlDoc.child(_T("div"));
	if(pDivElem == NULL)
	{
		DuiSystem::LogEvent(LOG_LEVEL_ERROR, L"CDuiPanel::LoadXmlFile %s failed, not found div node", strFileName);
		return FALSE;
	}

	// 加载div节点属性
	if(!Load(pDivElem))
	{
		DuiSystem::LogEvent(LOG_LEVEL_ERROR, L"CDuiPanel::LoadXmlFile %s failed, load div node fail", strFileName);
		return FALSE;
	}

	return TRUE;
}

// 从XML设置图片信息属性
HRESULT CDuiPanel::OnAttributeImageScroll(const CString& strValue, BOOL bLoading)
{
	if (strValue.IsEmpty()) return E_FAIL;

	// 通过Skin读取
	CString strSkin = _T("");
	if(strValue.Find(_T("skin:")) == 0)
	{
		strSkin = DuiSystem::Instance()->GetSkin(strValue);
		if (strSkin.IsEmpty()) return E_FAIL;
	}else
	{
		strSkin = strValue;
	}

	if(strSkin.Find(_T(".")) != -1)	// 加载图片文件
	{
		CString strImgFile = strSkin;
		if(strSkin.Find(_T(":")) != -1)
		{
			strImgFile = strSkin;
		}
		if(!m_pControScrollV->SetBitmap(strImgFile))
		{
			return E_FAIL;
		}
	}else	// 加载图片资源
	{
		UINT nResourceID = _wtoi(strSkin);
		if(!m_pControScrollV->SetBitmap(nResourceID, TEXT("PNG")))
		{
			if(!m_pControScrollV->SetBitmap(nResourceID, TEXT("BMP")))
			{
				return E_FAIL;
			}
		}
	}

	return bLoading?S_FALSE:S_OK;
}

// 从XML设置XML属性,加载XML文件的内容作为div容器内的内容
HRESULT CDuiPanel::OnAttributeXml(const CString& strValue, BOOL bLoading)
{
	if (strValue.IsEmpty()) return E_FAIL;

	DuiXmlDocument xmlDoc;
	DuiXmlNode pDivElem;

	if(DuiSystem::Instance()->LoadXmlFile(xmlDoc, strValue))
	{
		m_strXmlFile = strValue;
		pDivElem = xmlDoc.child(_T("div"));
		if(pDivElem != NULL)
		{
			// 加载div节点属性
			Load(pDivElem);
		}
	}

	return bLoading?S_FALSE:S_OK;
}

// 从XML设置plugin属性,加载界面插件文件动态库
HRESULT CDuiPanel::OnAttributePlugin(const CString& strValue, BOOL bLoading)
{
	if (strValue.IsEmpty()) return E_FAIL;

	HINSTANCE hPluginHandle = NULL;
	LPVOID pPluginObj = NULL;

	if(DuiSystem::Instance()->LoadPluginFile(strValue, CEncodingUtil::AnsiToUnicode(IID_IDuiPluginPanel), hPluginHandle, pPluginObj))
	{
		m_strPluginFile = strValue;
		m_hPluginHandle = hPluginHandle;
		m_pDuiPluginObject = (IDuiPluginPanel*)pPluginObj;
		// 初始化界面插件
		UINT nIDTemplate = 0;
		HWND hWnd = NULL;
		CDlgBase* pParentDlg = GetParentDialog();
		if(pParentDlg != NULL)
		{
			nIDTemplate = pParentDlg->GetIDTemplate();
			hWnd = pParentDlg->GetSafeHwnd();
		}
		m_pDuiPluginObject->OnInit(nIDTemplate, hWnd, CEncodingUtil::UnicodeToAnsi(GetName()), m_rc);
	}

	return bLoading?S_FALSE:S_OK;
}

// 初始化窗口控件
void CDuiPanel::InitUI(CRect rcClient, DuiXmlNode pNode)
{
	CRect rcTemp;
	int nStartX = 0;
	int nStartY = 0;
	CControlBase * pControlBase = NULL;

	// 加载所有窗口控件
	if(pNode)
	{
		m_nVirtualHeight = 0;
		for (DuiXmlNode pControlElem = pNode.first_child(); pControlElem; pControlElem=pControlElem.next_sibling())
		{
			if(pControlElem != NULL)
			{
				CString strControlName = pControlElem.name();
				CControlBase* pControl = _CreateControlByName(strControlName);
				if(pControl)
				{
					pControl->Load(pControlElem);
					if(pControl->IsClass(CArea::GetClassName()) || pControl->IsClass(CDuiFrame::GetClassName()))
					{
						// Area和Frame不能响应鼠标,必须加到Area列表中
						m_vecArea.push_back(pControl);
					}else
					{
						m_vecControl.push_back(pControl);
					}

					CRect rcCtrl = pControl->GetRect();
					if(rcCtrl.bottom > m_nVirtualHeight)
					{
						m_nVirtualHeight = rcCtrl.bottom - m_rc.top;
					}
				}
			}
		}

		// 需要的总高度大于显示区高度才会显示滚动条
		m_pControScrollV->SetVisible(m_nVirtualHeight > m_rc.Height());
		((CScrollV*)m_pControScrollV)->SetScrollMaxRange(m_nVirtualHeight);
	}

	m_bInit = true;
}

// 计算虚拟高度
void CDuiPanel::CalcVirtualHeight()
{
	CRect rcTemp;
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			rcTemp = pControlBase->GetRect();
			if(rcTemp.bottom > m_nVirtualHeight)
			{
				// 刷新Panel的虚拟高度
				m_nVirtualHeight = rcTemp.bottom - m_rc.top;
			}
		}
	}

	// 需要的总高度大于显示区高度才会显示滚动条
	m_pControScrollV->SetVisible(m_nVirtualHeight > m_rc.Height());
	((CScrollV*)m_pControScrollV)->SetScrollMaxRange(m_nVirtualHeight);
}

void CDuiPanel::SetControlRect(CRect rc)
{
	m_rc = rc;

	if(m_pDuiPluginObject)
	{
		m_pDuiPluginObject->SetRect(rc);
	}

	CRect rcTemp;
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			UINT uControlID = pControlBase->GetControlID();
			if(SCROLL_V == uControlID)
			{
				rcTemp = m_rc;
				rcTemp.left = rcTemp.right - m_nScrollWidth;
			}else
			{
				rcTemp = pControlBase->GetRect();
				if(rcTemp.bottom > m_nVirtualHeight)
				{
					// 刷新Panel的虚拟高度
					m_nVirtualHeight = rcTemp.bottom - m_rc.top;
				}
				continue;
			}
			pControlBase->SetRect(rcTemp);			
		}
	}

	// 需要的总高度大于显示区高度才会显示滚动条
	m_pControScrollV->SetVisible(m_nVirtualHeight > m_rc.Height());
	((CScrollV*)m_pControScrollV)->SetScrollMaxRange(m_nVirtualHeight);
}

// 重载设置控件可见性的函数，需要调用子控件的函数
void CDuiPanel::SetControlVisible(BOOL bIsVisible)
{
	__super::SetControlVisible(bIsVisible);

	// 设置每个子控件的原生Windows控件的可见性
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			if(pControlBase->IsClass(_T("div")) || pControlBase->IsClass(_T("tabctrl")))
			{
				pControlBase->SetControlVisible(bIsVisible);
			}else
			{
				// Panel可见性变化时候,只会隐藏原生控件,不主动显示原生控件
				//if(!bIsVisible)
				{
					pControlBase->SetControlWndVisible(bIsVisible);
				}
			}
		}
	}

	if(m_pDuiPluginObject)
	{
		m_pDuiPluginObject->SetVisible(bIsVisible);
	}
}

// 重载设置控件是否禁用的函数，需要调用子控件的函数
void CDuiPanel::SetControlDisable(BOOL bIsDisable)
{
	__super::SetControlDisable(bIsDisable);

	// 设置每个子控件的原生Windows控件的可见性
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase)
		{
			pControlBase->SetControlDisable(bIsDisable);
		}
	}

	if(m_pDuiPluginObject)
	{
		m_pDuiPluginObject->SetDisable(bIsDisable);
	}
}

// 重载设置控件焦点状态的函数
BOOL CDuiPanel::SetControlFocus(BOOL bFocus)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->SetFocus(bFocus);
	}

	return __super::SetControlFocus(bFocus);
}

void CDuiPanel::DrawControl(CDC &dc, CRect rcUpdate)
{
	if(m_pDuiPluginObject)
	{
		m_pDuiPluginObject->DrawControl(dc, rcUpdate);
	}
}

// 画子控件(支持滚动显示)
BOOL CDuiPanel::DrawSubControls(CDC &dc, CRect rcUpdate)
{
	// 如果不需要滚动条,则直接使用父控件的画图方法
	if(!m_bEnableScroll || (m_nVirtualHeight < m_rc.Height()))
	{
		return __super::DrawSubControls(dc, rcUpdate);
	}

	// 计算显示位置
	CScrollV* pScrollV = (CScrollV*)m_pControScrollV;
	int nCurPos = pScrollV->GetScrollCurrentPos();	// 当前top位置
	int nMaxRange = pScrollV->GetScrollMaxRange();
	int nVirtualTop = (nMaxRange > 0) ? nCurPos*(m_nVirtualHeight-m_rc.Height())/nMaxRange : 0;	// 当前显示的是虚拟图片中什么位置开始的图片

	// 根据滚动条位置画子控件
	// 初始化虚拟显示dc,并复制背景到虚拟显示dc
	CBitmap	virtualBitmap;
	CDC virtualDC;
	virtualDC.CreateCompatibleDC(&dc);
	virtualBitmap.CreateCompatibleBitmap(&dc, m_rc.right-m_nScrollWidth, m_rc.top + m_nVirtualHeight);
	CBitmap* pOldVirtualBitmap = virtualDC.SelectObject(&virtualBitmap);
	virtualDC.BitBlt(m_rc.left, m_rc.top+nVirtualTop, m_rc.Width()-m_nScrollWidth, m_rc.Height(), &dc, m_rc.left, m_rc.top, SRCCOPY);	// 背景复制到虚拟显示dc

	// 更新区域按照显示区域调整
	rcUpdate.OffsetRect(0, nVirtualTop);

	// 画子控件到虚拟dc
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase && pControlBase->GetVisible() && (pControlBase != m_pControScrollV))
		{
			// 如果顶部位置发生了变化,需要重画所有子控件,但会造成速度很慢
			if(m_nVirtualTop != nVirtualTop)
			{
				pControlBase->UpdateControl(true);
			}
			pControlBase->Draw(virtualDC, rcUpdate);		
		}
	}

	m_pControScrollV->Draw(dc, rcUpdate);

	m_nVirtualTop = nVirtualTop;

	// 虚拟显示dc复制到dc
	dc.BitBlt(m_rc.left, m_rc.top, m_rc.Width()-m_nScrollWidth, m_rc.Height(), &virtualDC, m_rc.left, m_rc.top + nVirtualTop, SRCCOPY);

	// 释放虚拟显示dc
	virtualDC.SelectObject(pOldVirtualBitmap);
	virtualDC.DeleteDC();
	virtualBitmap.DeleteObject();

	return TRUE;
}

// 鼠标坐标变换
BOOL CDuiPanel::OnMousePointChange(CPoint& point)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnMousePointChange(point);
	}

	if(m_bEnableScroll && (m_nVirtualHeight > m_rc.Height()))
	{
		// 计算显示位置
		CScrollV* pScrollV = (CScrollV*)m_pControScrollV;
		int nCurPos = pScrollV->GetScrollCurrentPos();	// 当前top位置
		int nMaxRange = pScrollV->GetScrollMaxRange();
		int nVirtualTop = (nMaxRange > 0) ? nCurPos*(m_nVirtualHeight-m_rc.Height())/nMaxRange : 0;	// 当前显示的是虚拟图片中什么位置开始的图片

		point.Offset(0, nVirtualTop);

		return TRUE;
	}

	return FALSE;
}

// 判断鼠标是否在控件可响应的区域
BOOL CDuiPanel::OnCheckMouseResponse(UINT nFlags, CPoint point)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnCheckMouseResponse(nFlags, point);
	}
	return TRUE;
}

// 鼠标移动事件处理
BOOL CDuiPanel::OnControlMouseMove(UINT nFlags, CPoint point)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnMouseMove(nFlags, point);
	}
	return FALSE;
}

// 鼠标左键按下事件处理
BOOL CDuiPanel::OnControlLButtonDown(UINT nFlags, CPoint point)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnLButtonDown(nFlags, point);
	}
	return FALSE;
}

// 鼠标左键放开事件处理
BOOL CDuiPanel::OnControlLButtonUp(UINT nFlags, CPoint point)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnLButtonUp(nFlags, point);
	}
	return FALSE;
}

// 鼠标左键双击事件处理
BOOL CDuiPanel::OnControlLButtonDblClk(UINT nFlags, CPoint point)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnLButtonDblClk(nFlags, point);
	}
	return FALSE;
}

// 键盘事件处理
BOOL CDuiPanel::OnControlKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnKeyDown(nChar, nRepCnt, nFlags);
	}
	return FALSE;
}

// 定时器事件处理
BOOL CDuiPanel::OnControlTimer()
{
	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnTimer(0, "");
	}
	return __super::OnControlTimer();
}

// 滚动事件处理
BOOL CDuiPanel::OnControlScroll(BOOL bVertical, UINT nFlags, CPoint point)
{
	if(m_pDuiPluginObject)
	{
		m_pDuiPluginObject->OnScroll(bVertical, nFlags, point);
	}

	if(!m_bEnableScroll || (m_nVirtualHeight < m_rc.Height()))
	{
		return false;
	}

	// 更新滚动条,并刷新界面
	CScrollV* pScroll = (CScrollV*)m_pControScrollV;
	if(pScroll->ScrollRow((nFlags == SB_LINEDOWN) ? 1 : -1))
	{
		UpdateControl(true);
		// 刷新滚动条
		pScroll->UpdateControl(true);
	}

	return true;
}

// 消息响应
LRESULT CDuiPanel::OnBaseMessage(UINT uID, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return OnMessage(uID, uMsg, wParam, lParam);
}

// 控件界面刷新
LRESULT CDuiPanel::OnControlUpdate(CRect rcUpdate, BOOL bUpdate, CControlBase *pUpdateControlBase) 
{
	if(pUpdateControlBase == NULL) return 0;

	if(bUpdate)
	{
		pUpdateControlBase->SetUpdate(FALSE);
	}

	int nVirtualTop = 0;
	if(m_nVirtualHeight > m_rc.Height())
	{
		// 计算显示位置
		CScrollV* pScrollV = (CScrollV*)m_pControScrollV;
		int nCurPos = pScrollV->GetScrollCurrentPos();	// 当前top位置
		int nMaxRange = pScrollV->GetScrollMaxRange();
		nVirtualTop = (nMaxRange > 0) ? nCurPos*(m_nVirtualHeight-m_rc.Height())/nMaxRange : 0;	// 当前显示的是虚拟图片中什么位置开始的图片
	}

	CRect rcAllUpDate = rcUpdate;

	if(m_bInit)
	{
		BOOL bFind = false;

		for (size_t i = 0; i < m_vecControl.size(); i++)
		{
			CControlBase * pControlBase = m_vecControl.at(i);
			if (pControlBase)
			{
				if(bFind)
				{
					if (pControlBase->GetVisible() && !(pControlBase->GetRect() & rcUpdate).IsRectEmpty())
					{
						rcAllUpDate |= pControlBase->GetRect();
						pControlBase->SetUpdate(FALSE);
					}
				}
				else if(pControlBase == pUpdateControlBase)
				{
					bFind = true;
				}
			}
		}
	}

	// 按照滚动位置修改更新区域
	rcAllUpDate.OffsetRect(0, -nVirtualTop);
	rcAllUpDate |= m_rc;

	InvalidateRect(&rcAllUpDate);

	return 0L; 
};

// 消息处理
LRESULT CDuiPanel::OnMessage(UINT uID, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// 遍历每个控件，看哪个控件是此ID，则进行事件的处理
	for (size_t i = 0; i < m_vecControl.size(); i++)
	{
		CControlBase * pControlBase = m_vecControl.at(i);
		if (pControlBase && (pControlBase->GetControlID() == uID))
		{
			return pControlBase->OnMessage(uID, Msg, wParam, lParam);
		}
	}

	if(m_pDuiPluginObject)
	{
		return m_pDuiPluginObject->OnMessage(uID, Msg, wParam, lParam);
	}

	// 如果事件未处理,则调用父类的消息函数,最终会送给各事件处理对象进行处理
	return __super::OnMessage(uID, Msg, wParam, lParam);
}