#include "DocumentView.h"
#include "viewStyles.h"

extern onut::UIControl* g_pUIScreen;

DocumentView::DocumentView()
{
    pUIContext = new onut::UIContext(onut::sUIVector2{640, 480});
    createViewUIStyles(pUIContext);

    pUIScreen = new onut::UIControl();
    pUIScreen->setWidthType(onut::eUIDimType::DIM_RELATIVE);
    pUIScreen->setHeightType(onut::eUIDimType::DIM_RELATIVE);

    // Dotted line gizmo for selection
    m_pGizmo = g_pUIScreen->getChild<onut::UIPanel>("gizmo");
    m_pGizmo->retain();
    m_gizmoHandles[0] = m_pGizmo->getChild("topLeftHandle");
    m_gizmoHandles[1] = m_pGizmo->getChild("topHandle");
    m_gizmoHandles[2] = m_pGizmo->getChild("topRightHandle");
    m_gizmoHandles[3] = m_pGizmo->getChild("leftHandle");
    m_gizmoHandles[4] = m_pGizmo->getChild("rightHandle");
    m_gizmoHandles[5] = m_pGizmo->getChild("bottomLeftHandle");
    m_gizmoHandles[6] = m_pGizmo->getChild("bottomHandle");
    m_gizmoHandles[7] = m_pGizmo->getChild("bottomRightHandle");
    static HCURSOR curARROW = LoadCursor(nullptr, IDC_ARROW);
    static HCURSOR curSIZENWSE = LoadCursor(nullptr, IDC_SIZENWSE);
    static HCURSOR curSIZENESW = LoadCursor(nullptr, IDC_SIZENESW);
    static HCURSOR curSIZEWE = LoadCursor(nullptr, IDC_SIZEWE);
    static HCURSOR curSIZENS = LoadCursor(nullptr, IDC_SIZENS);
    static HCURSOR curSIZEALL = LoadCursor(nullptr, IDC_SIZEALL);
    m_pGizmo->onMouseDown = std::bind(&DocumentView::onGizmoStart, this, std::placeholders::_1, std::placeholders::_2);
    m_pGizmo->onMouseUp = std::bind(&DocumentView::onGizmoEnd, this, std::placeholders::_1, std::placeholders::_2);
    m_pGizmo->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZEALL); };
    m_pGizmo->onMouseLeave = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curARROW); };
    for (auto& pGizmoHandle : m_gizmoHandles)
    {
        pGizmoHandle->onMouseDown = std::bind(&DocumentView::onGizmoHandleStart, this, std::placeholders::_1, std::placeholders::_2);
        pGizmoHandle->onMouseUp = std::bind(&DocumentView::onGizmoHandleEnd, this, std::placeholders::_1, std::placeholders::_2);
        pGizmoHandle->onMouseLeave = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curARROW); };
    }
    m_gizmoHandles[0]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZENWSE); };
    m_gizmoHandles[1]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZENS); };
    m_gizmoHandles[2]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZENESW); };
    m_gizmoHandles[3]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZEWE); };
    m_gizmoHandles[4]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZEWE); };
    m_gizmoHandles[5]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZENESW); };
    m_gizmoHandles[6]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZENS); };
    m_gizmoHandles[7]->onMouseEnter = [&](onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent){OWindow->setCursor(curSIZENWSE); };

    // Add some dummy nodes in the scene graph for show
    m_pSceneGraph = g_pUIScreen->getChild<onut::UITreeView>("sceneGraph");

    // Transform handles
    const float HANDLE_SIZE = 6.f;
    const float HANDLE_PADDING = 3.f;
}

DocumentView::~DocumentView()
{
    m_pGizmo->release();
    delete pUIScreen;
    delete pUIContext;
}

void DocumentView::controlCreated(onut::UIControl* pControl, onut::UIControl* pParent)
{
    auto pItem = new onut::UITreeViewItem();
    pItem->setUserData(pControl);
    pControl->setUserData(pItem);
    auto pParentItem = static_cast<onut::UITreeViewItem*>(pParent->getUserData());
    if (pParentItem)
    {
        pParentItem->addItem(pItem);
    }
    else
    {
        m_pSceneGraph->addItem(pItem);
    }
}

void DocumentView::setSelected(onut::UIControl* in_pSelected, bool bUpdateSceneGraph)
{ 
    if (bUpdateSceneGraph)
    {
        m_pSceneGraph->unselectAll();
    }
    if (in_pSelected == pUIScreen)
    {
        pSelected = nullptr;
    }
    else
    {
        pSelected = in_pSelected;
    }
    m_pGizmo->setIsVisible(pSelected != nullptr);
    if (pSelected)
    {
        if (bUpdateSceneGraph)
        {
            auto pItem = static_cast<onut::UITreeViewItem*>(pSelected->getUserData());
            m_pSceneGraph->addSelectedItem(pItem);
        }
        m_pGizmo->setRect(pSelected->getWorldRect(*pUIContext));
    }
    updateInspector();
}

void DocumentView::onGizmoHandleStart(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent)
{
    m_state = eDocumentState::MOVING_HANDLE;
    m_mousePosOnDown = {mouseEvent.mousePos.x, mouseEvent.mousePos.y};
    m_pCurrentHandle = pControl;
    m_gizmoRectOnDown = pSelected->getWorldRect(*pUIContext);
}

void DocumentView::onGizmoHandleEnd(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent)
{
    m_state = eDocumentState::IDLE;
}

void DocumentView::onGizmoStart(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent)
{
    static HCURSOR curSIZEALL = LoadCursor(nullptr, IDC_SIZEALL);
    OWindow->setCursor(curSIZEALL);
    m_state = eDocumentState::MOVING_GIZO;
    m_mousePosOnDown = {mouseEvent.mousePos.x, mouseEvent.mousePos.y};
    m_gizmoRectOnDown = pSelected->getWorldRect(*pUIContext);
}

void DocumentView::onGizmoEnd(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent)
{
    m_state = eDocumentState::IDLE;
}

void DocumentView::update()
{
    // We resize, but we don't update the screen. It's render only
    pUIContext->resize({OScreenWf, OScreenHf});

    switch (m_state)
    {
        case eDocumentState::MOVING_HANDLE:
            updateMovingHandle();
            break;
        case eDocumentState::MOVING_GIZO:
            updateMovingGizmo();
            break;
    }
}

void DocumentView::updateSelectionWithRect(const onut::sUIRect& rect)
{
    if (!pSelected) return;
    pSelected->setWorldRect(*pUIContext, rect);
}

void DocumentView::updateMovingGizmo()
{
    auto mouseDiff = OMousePos - m_mousePosOnDown;
    auto newRect = m_gizmoRectOnDown;
    newRect.position.x += mouseDiff.x;
    newRect.position.y += mouseDiff.y;
    m_pGizmo->setRect(newRect);
    updateSelectionWithRect(newRect);
}

void DocumentView::updateMovingHandle()
{
    auto mouseDiff = OMousePos - m_mousePosOnDown;
    if (m_pCurrentHandle == m_gizmoHandles[0])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.position.x += mouseDiff.x;
        newRect.position.x = std::min<float>(newRect.position.x, m_gizmoRectOnDown.position.x + m_gizmoRectOnDown.size.x);
        newRect.size.x -= mouseDiff.x;
        if (newRect.size.x < 0) newRect.size.x = 0;

        newRect.position.y += mouseDiff.y;
        newRect.position.y = std::min<float>(newRect.position.y, m_gizmoRectOnDown.position.y + m_gizmoRectOnDown.size.y);
        newRect.size.y -= mouseDiff.y;
        if (newRect.size.y < 0) newRect.size.y = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[1])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.position.y += mouseDiff.y;
        newRect.position.y = std::min<float>(newRect.position.y, m_gizmoRectOnDown.position.y + m_gizmoRectOnDown.size.y);
        newRect.size.y -= mouseDiff.y;
        if (newRect.size.y < 0) newRect.size.y = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[2])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.size.x += mouseDiff.x;
        if (newRect.size.x < 0) newRect.size.x = 0;

        newRect.position.y += mouseDiff.y;
        newRect.position.y = std::min<float>(newRect.position.y, m_gizmoRectOnDown.position.y + m_gizmoRectOnDown.size.y);
        newRect.size.y -= mouseDiff.y;
        if (newRect.size.y < 0) newRect.size.y = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[3])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.position.x += mouseDiff.x;
        newRect.position.x = std::min<float>(newRect.position.x, m_gizmoRectOnDown.position.x + m_gizmoRectOnDown.size.x);
        newRect.size.x -= mouseDiff.x;
        if (newRect.size.x < 0) newRect.size.x = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[4])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.size.x += mouseDiff.x;
        if (newRect.size.x < 0) newRect.size.x = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[5])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.position.x += mouseDiff.x;
        newRect.position.x = std::min<float>(newRect.position.x, m_gizmoRectOnDown.position.x + m_gizmoRectOnDown.size.x);
        newRect.size.x -= mouseDiff.x;
        if (newRect.size.x < 0) newRect.size.x = 0;

        newRect.size.y += mouseDiff.y;
        if (newRect.size.y < 0) newRect.size.y = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[6])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.size.y += mouseDiff.y;
        if (newRect.size.y < 0) newRect.size.y = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
    else if (m_pCurrentHandle == m_gizmoHandles[7])
    {
        auto newRect = m_gizmoRectOnDown;

        newRect.size.x += mouseDiff.x;
        if (newRect.size.x < 0) newRect.size.x = 0;

        newRect.size.y += mouseDiff.y;
        if (newRect.size.y < 0) newRect.size.y = 0;

        m_pGizmo->setRect(newRect);
        updateSelectionWithRect(newRect);
    }
}

void DocumentView::render()
{
    pUIScreen->render(*pUIContext);
}

void DocumentView::updateInspector()
{
}
