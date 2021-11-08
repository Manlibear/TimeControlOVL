#define TESLA_INIT_IMPL
#include <tesla.hpp>

TimeServiceType __nx_time_service_type = TimeServiceType_System; // so we can change the time

class GuiTest : public tsl::Gui
{

    typedef struct
    {
        u64 main_nso_base;
        u64 heap_base;
        u64 titleID;
        u8 buildID[0x20];
    } MetaData;

public:
    GuiTest() {}
    bool isError = false;

    const unsigned long ACNH_TimeAddress = 0x0BD29188;
    const unsigned long ACNH_TimeStateAddress = 0x00328530;
    const u64 ACNH_TitleID = 0x01006F8002326000;

    Handle debughandle = 0;
    int dayChange = 0, hourChange = 0, monthChange = 0;
    bool keyHandled = false;
    time_t timeToSet;
    struct tm *p_tm_timeToSet;
    tsl::elm::OverlayFrame *frame;
    tsl::elm::ListItem *currentTimeLabel = new tsl::elm::ListItem("Current Time");

    virtual tsl::elm::Element *createUI() override
    {
        frame = new tsl::elm::OverlayFrame("Time Control", "v1.1.0");

        auto list = new tsl::elm::List();

        list->addItem(currentTimeLabel);

        // give up some space
        list->addItem(drawStringLine(""), 70);

        list->addItem(drawStringLine("\uE0AC / \uE0AD  -  Change hour\n\n"), 32);
        list->addItem(drawStringLine("\uE0AE / \uE0AB  -  Change day\n\n"), 32);
        list->addItem(drawStringLine("\uE0E4 / \uE0E5  -  Change month\n\n"), 32);
        list->addItem(drawStringLine("\uE0E0          -  Update time"), 32);

        frame->setContent(list);

        return frame;
    }

    tsl::elm::CustomDrawer *drawStringLine(const char *text)
    {
        return new tsl::elm::CustomDrawer([text](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h)
                                          { renderer->drawString(
                                                text, false, x + 20, y + 20, 18, renderer->a(tsl::style::color::ColorDescription)); });
    }

    virtual void update() override
    {
        time_t currentTime;
        Result rs = timeGetCurrentTime(TimeType_UserSystemClock, (u64 *)&currentTime);

        if (R_FAILED(rs))
        {
            currentTimeLabel->setText("Unable to get time");
        }
        else
        {
            p_tm_timeToSet = localtime(&currentTime);
            p_tm_timeToSet->tm_hour += hourChange;
            p_tm_timeToSet->tm_mday += dayChange;
            p_tm_timeToSet->tm_mon += monthChange;
            timeToSet = mktime(p_tm_timeToSet);

            char timeToSetStr[25];
            strftime(timeToSetStr, sizeof timeToSetStr, "%a  %d/%m/%Y  %H:%M", p_tm_timeToSet);
            currentTimeLabel->setText(timeToSetStr);
        }
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick) override
    {
        if (!this->keyHandled)
        {
            if (keysDown & HidNpadButton_L)
            {
                this->monthChange--;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_R)
            {
                this->monthChange++;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Left)
            {
                this->dayChange--;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Right)
            {
                this->dayChange++;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Up)
            {
                this->hourChange++;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Down)
            {
                this->hourChange--;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_A)
            {
                isError = false;
                this->keyHandled = true;
                timeSetCurrentTime(TimeType_NetworkSystemClock, (uint64_t)timeToSet);

                this->hourChange = 0;
                this->dayChange = 0;
                this->monthChange = 0;

                MetaData meta = getMetaData();

                if (meta.titleID == ACNH_TitleID)
                {
                    u8 bFreeze[4]{31, 32, 3, 213};

                    int year = p_tm_timeToSet->tm_year + 1900;
                    u8 yearByteUpper = (year >> 8) & 0xFF;
                    u8 yearByteLower = year & 0xFF;

                    u8 bTime[6]{
                        yearByteLower, yearByteUpper,
                        (u8)(p_tm_timeToSet->tm_mon + 1),
                        (u8)p_tm_timeToSet->tm_mday,
                        (u8)p_tm_timeToSet->tm_hour,
                        (u8)p_tm_timeToSet->tm_min};

                    poke(meta.heap_base + ACNH_TimeStateAddress, 4, bFreeze);
                    poke(meta.heap_base + ACNH_TimeAddress, 6, bTime);
                }
            }

            return this->keyHandled;
        }

        if (keysDown == 0)
        {
            this->keyHandled = false;
        }

        return false;
    }

    MetaData getMetaData()
    {
        MetaData meta;
        attach();
        u64 pid = 0;
        Result rc = pmdmntGetApplicationProcessId(&pid);
        if (R_FAILED(rc))
            drawErrorSubtitle("pmdmntGetApplicationProcessId: " + std::to_string(rc));

        meta.heap_base = getHeapBase(debughandle);
        meta.titleID = getTitleId(pid);

        detach();

        return meta;
    }

    void poke(u64 offset, u64 size, u8 *val)
    {
        attach();
        writeMem(offset, size, val);
        detach();
    }

    void writeMem(u64 offset, u64 size, u8 *val)
    {
        Result rc = svcWriteDebugProcessMemory(debughandle, val, offset, size);
        if (R_FAILED(rc))
            drawErrorSubtitle("svcWriteDebugProcessMemory: " + std::to_string(rc));
    }

    void drawErrorSubtitle(std::string err)
    {
        if (!isError)
        {
            isError = true;
            frame->setSubtitle(err);
        }
    }

    u64 getHeapBase(Handle handle)
    {
        MemoryInfo meminfo;
        memset(&meminfo, 0, sizeof(MemoryInfo));
        u64 heap_base = 0;
        u64 lastaddr = 0;
        do
        {
            lastaddr = meminfo.addr;
            u32 pageinfo;
            svcQueryDebugProcessMemory(&meminfo, &pageinfo, handle, meminfo.addr + meminfo.size);
            if ((meminfo.type & MemType_Heap) == MemType_Heap)
            {
                heap_base = meminfo.addr;
                break;
            }
        } while (lastaddr < meminfo.addr + meminfo.size);

        return heap_base;
    }

    u64 getTitleId(u64 pid)
    {
        u64 titleId = 0;
        Result rc = pminfoGetProgramId(&titleId, pid);
        if (R_FAILED(rc))
            drawErrorSubtitle("pminfoGetProgramId: " + std::to_string(rc));
        return titleId;
    }

    void attach()
    {
        u64 pid = 0;
        Result rc = pmdmntGetApplicationProcessId(&pid);
        if (R_FAILED(rc))
            drawErrorSubtitle("pmdmntGetApplicationProcessId: " + std::to_string(rc));

        if (debughandle != 0)
            svcCloseHandle(debughandle);

        rc = svcDebugActiveProcess(&debughandle, pid);
        if (R_FAILED(rc))
            drawErrorSubtitle("svcDebugActiveProcess: " + std::to_string(rc));
    }

    void detach()
    {
        if (debughandle != 0)
            svcCloseHandle(debughandle);
    }
};

class OverlayTest : public tsl::Overlay
{
public:
    virtual void initServices() override
    {
        timeInitialize();
        pminfoInitialize();
        pmdmntInitialize();
    }

    virtual void exitServices() override
    {
        timeExit();
        pminfoExit();
        pmdmntExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override
    {
        return initially<GuiTest>();
    }
};

int main(int argc, char **argv)
{
    return tsl::loop<OverlayTest>(argc, argv);
}