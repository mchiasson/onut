#pragma once
#include "onut.h"

enum class eDocumentState : uint8_t
{
    IDLE,
    MOVING_GIZO,
    MOVING_HANDLE
};

class DocumentView
{
public:
    DocumentView(const std::string& filename);
    virtual ~DocumentView();

    void setSelected(onut::UIControl* in_pSelected, bool bUpdateSceneGraph = true);

    void update();
    void render();
    void save();

    onut::UIContext*            pUIContext = nullptr;
    onut::UIControl*            pUIScreen = nullptr;
    onut::UIControl*            pSelected = nullptr;
    onut::ContentManager<false> contentManager;

    void onGizmoStart(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent);
    void onGizmoEnd(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent);

    void controlCreated(onut::UIControl* pControl, onut::UIControl* pParent);
    void controlDeleted(onut::UIControl* pControl);
    void updateSelectedGizmoRect();
    void updateInspector();
    void repopulateTreeView(onut::UIControl* pControl);
    void onKeyDown(uintptr_t key);

    bool isBusy() const { return m_state != eDocumentState::IDLE; }

    const std::string& getFilename() const { return m_filename; }

private:
    void onGizmoHandleStart(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent);
    void onGizmoHandleEnd(onut::UIControl* pControl, const onut::UIMouseEvent& mouseEvent);

    void updateSelectionWithRect(const onut::sUIRect& rect);
    void updateGizmoRect();
    void updateMovingHandle();
    void updateMovingGizmo();
    void deleteSelection();
    void setDirty(bool isDirty);
    void concludeTransform(onut::UIControl* pControl, const onut::sUIRect& previousRect);

    void snapX(float x, float &ret, const onut::sUIRect& rect, float &closest, bool& found);
    float snapX(onut::UIControl* pControl, float x);
    void snapY(float y, float &ret, const onut::sUIRect& rect, float &closest, bool& found);
    float snapY(onut::UIControl* pControl, float y);
    bool getXAutoGuide(const onut::sUIRect& rect, float& x, bool& side);
    bool getYAutoGuide(const onut::sUIRect& rect, float& y, bool& side);
    void xAutoGuideAgainst(const onut::sUIRect& otherRect, bool& found, const onut::sUIRect& rect, float& x, bool& side, float& closest);
    void yAutoGuideAgainst(const onut::sUIRect& otherRect, bool& found, const onut::sUIRect& rect, float& y, bool& side, float& closest);

    onut::UIPanel*      m_pGizmo = nullptr;
    onut::UIControl*    m_gizmoHandles[8];
    onut::UIControl*    m_guides[2];
    onut::UIControl*    m_pCurrentHandle = nullptr;
    onut::UITreeView*   m_pSceneGraph = nullptr;
    Vector2             m_mousePosOnDown;
    onut::sUIRect       m_controlRectOnDown;
    onut::sUIRect       m_controlWorldRectOnDown;
    eDocumentState      m_state = eDocumentState::IDLE;
    bool                m_autoGuide = true;
    float               m_autoPadding = 8.f;
    bool                m_isDirty = false;
    std::string         m_filename;
};

class IControlInspectorBind
{
public:
    virtual void updateInspector(onut::UIControl* pControl) = 0;
    virtual void updateControl(onut::UIControl* pControl) = 0;
};

template<typename Ttype, typename TtargetControl, typename TgetterFn, typename TsetterFn>
class ControlInspectorBind : public IControlInspectorBind
{
public:
    ControlInspectorBind(const std::string& actionName,
                         std::function<const Ttype&()> inspectorGetter,
                         std::function<void(const Ttype&)> inspectorSetter,
                         TgetterFn getter,
                         TsetterFn setter) :
                         m_actionName(actionName),
                         m_inspectorGetter(inspectorGetter),
                         m_inspectorSetter(inspectorSetter),
                         m_getter(getter),
                         m_setter(setter)
    {
    }

private:
    std::string                         m_actionName;
    std::function<const Ttype&()>       m_inspectorGetter;
    std::function<void(const Ttype&)>   m_inspectorSetter;
    TgetterFn                           m_getter;
    TsetterFn                           m_setter;

public:
    void updateInspector(onut::UIControl* pControl) override
    {
        auto tControl = dynamic_cast<TtargetControl*>(pControl);
        if (!tControl) return;
        if (!m_inspectorSetter) return;
        m_inspectorSetter(m_getter(tControl));
    }

    void updateControl(onut::UIControl* pControl) override
    {
        auto tControl = dynamic_cast<TtargetControl*>(pControl);
        if (!tControl) return;
        if (!m_inspectorGetter) return;

        auto prevVar = m_getter(tControl);
        auto newVal = m_inspectorGetter();

        g_actionManager.doAction(new onut::Action(m_actionName,
            [=]{
            m_setter(tControl, newVal);
            g_pDocument->updateInspector();
        },
            [=]{
            m_setter(tControl, prevVar);
            g_pDocument->updateInspector();
        },
            [=]{
            tControl->retain();
        },
            [=]{
            tControl->release();
        }));
    }
};
