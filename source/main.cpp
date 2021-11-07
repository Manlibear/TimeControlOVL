#define TESLA_INIT_IMPL
#include <tesla.hpp>


TimeServiceType __nx_time_service_type = TimeServiceType_System; // so we can change the time

class GuiTest : public tsl::Gui {

public:
    GuiTest() { }

    tsl::elm::ListItem* currentTimeLabel = new tsl::elm::ListItem("Current Time");
    int dayChange = 0;
    int hourChange = 0;
    int monthChange = 0;
    bool keyHandled = false;
    time_t timeToSet;
    tsl::elm::OverlayFrame* frame;

    bool setNetworkSystemClock(time_t time) {
        Result rs = timeSetCurrentTime(TimeType_NetworkSystemClock, (uint64_t)time);
        if (R_FAILED(rs)) {
            return false;
        }
        return true;
    }
    
    virtual tsl::elm::Element* createUI() override {
        frame = new tsl::elm::OverlayFrame("Time Control", "v1.0.0");
        auto list = new tsl::elm::List();

        list->addItem(currentTimeLabel);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            // blank space
        }),
        70);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(
            "\uE0AC / \uE0AD  -  Change hour\n\n", false, x + 20, y + 20, 18, renderer->a(tsl::style::color::ColorDescription));
        }),
        32);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(
            "\uE0AE / \uE0AB  -  Change day\n\n", false, x + 20, y + 20, 18, renderer->a(tsl::style::color::ColorDescription));
        }),
        32);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(
            "\uE0E4 / \uE0E5  -  Change month\n\n", false, x + 20, y + 20, 18, renderer->a(tsl::style::color::ColorDescription));
        }),
        32);

        list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(
            "\uE0E0          -  Update time", false, x + 20, y + 20, 18, renderer->a(tsl::style::color::ColorDescription));
        }),
        32);
        
        frame->setContent(list);

        return frame;
    }

    virtual void update() override {
        
        time_t currentTime;
        Result rs = timeGetCurrentTime(TimeType_UserSystemClock, (u64*)&currentTime);

        if(R_FAILED(rs)){
            
            currentTimeLabel->setText("Unable to get time");
            // currentTimeLabel->setValue(format("0x%x", rs));
        }
        else
        {
            struct tm* p_tm_timeToSet = localtime(&currentTime);
            p_tm_timeToSet->tm_mday += dayChange;
            p_tm_timeToSet->tm_hour += hourChange;
            p_tm_timeToSet->tm_mon += monthChange;
            timeToSet = mktime(p_tm_timeToSet);
            
            char timeToSetStr[25];
            strftime(timeToSetStr, sizeof timeToSetStr, "%a  %d/%m/%Y  %H:%M", p_tm_timeToSet);
            currentTimeLabel->setText(timeToSetStr);
        }
    }

   virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick) override{
        
       if(!this->keyHandled){
           
           if (keysDown & HidNpadButton_L) {
                this->monthChange--;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_R) {
                this->monthChange++;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Left) {
                this->dayChange--;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Right) {
                this->dayChange++;
                this->keyHandled = true;
            }
            
            if (keysDown & HidNpadButton_Up) {
                this->hourChange++;
                this->keyHandled = true;
            }

            if (keysDown & HidNpadButton_Down) {
                this->hourChange--;
                this->keyHandled = true;
            }

            if(keysDown & HidNpadButton_A){
                this->keyHandled = true;
                Result rs = timeSetCurrentTime(TimeType_NetworkSystemClock, (uint64_t)timeToSet);

                if(R_FAILED(rs)){
                    // frame->setSubtitle(format("Unable to set time: 0x%x", rs));
                } else
                {    
                    this->hourChange = 0;
                    this->dayChange = 0;
                    this->monthChange = 0;
                }
            }

            return this->keyHandled;
       }

       if(keysDown == 0){
           this->keyHandled = false;
       }

        return false;
    }
};

class OverlayTest : public tsl::Overlay {
public:
    virtual void initServices() override {
        
        timeInitialize();

    }
    
    virtual void exitServices() override {
        timeExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiTest>();
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}