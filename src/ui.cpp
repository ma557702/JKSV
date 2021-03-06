#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <switch.h>

#include "ui.h"
#include "gfx.h"
#include "util.h"
#include "file.h"

#define VER_STRING "v. 05.15.2020"

//Don't waste time drawing top and bottom over and over
//guide graphics are to save cpu drawing that over and over with alpha
static tex *top, *bot, *usrGuide, *ttlGuide, *fldrGuide, *optGuide;

//Ui text strings
std::string author, ui::userHelp, ui::titleHelp, ui::folderHelp, ui::optHelp;
std::string ui::confBlackList, ui::confOverwrite, ui::confRestore, ui::confDel, ui::confCopy;
std::string ui::confEraseNand, ui::confEraseFolder;

//X position of help texts. Calculated to make editing quicker/easier
static unsigned userHelpX, titleHelpX, folderHelpX, optHelpX;

static void loadTrans()
{
    std::string file;
    if(fs::fileExists(fs::getWorkDir() + "trans.txt"))
        file = fs::getWorkDir() + "trans.txt";
    else
    {
        file = "romfs:/lang/";
        switch(data::sysLang)
        {
            default:
                file += "en-US.txt";
                break;
        }
    }

    fs::dataFile lang(file);
    author = lang.getNextLine();
    if(author == "NULL")
        author = "";

    ui::userHelp = lang.getNextLine();
    ui::titleHelp = lang.getNextLine();
    ui::folderHelp = lang.getNextLine();
    ui::optHelp = lang.getNextLine();
    ui::confBlackList = lang.getNextLine();
    ui::confOverwrite = lang.getNextLine();
    ui::confRestore = lang.getNextLine();
    ui::confDel = lang.getNextLine();
    ui::confCopy = lang.getNextLine();
    ui::confEraseNand = lang.getNextLine();
    ui::confEraseFolder = lang.getNextLine();
}

namespace ui
{
    //text mode
    bool textMode = false;

    //Current menu state
    int mstate = USR_SEL, prevState = USR_SEL;

    //Theme id
    ColorSetId thmID;

    //Info printed on folder menu
    std::string folderMenuInfo;

    //UI colors
    clr clearClr, txtCont, txtDiag, rectLt, rectSh, tboxClr, divClr;

    //textbox pieces
    //I was going to flip them when I draw them, but then laziness kicked in.
    tex *cornerTopLeft, *cornerTopRight, *cornerBottomLeft, *cornerBottomRight;
    tex *progCovLeft, *progCovRight, *diaBox;

    //Menu box pieces
    tex *mnuTopLeft, *mnuTopRight, *mnuBotLeft, *mnuBotRight;

    //Select box + top left icon
    tex *icn, *sideBar;

    //Shared font
    font *shared;

    void initTheme()
    {
        if(fs::fileExists(fs::getWorkDir() + "font.ttf"))
            shared = fontLoadTTF(std::string(fs::getWorkDir() + "font.ttf").c_str());
        else
            shared = fontLoadSharedFonts();

        setsysGetColorSetId(&thmID);

        switch(thmID)
        {
            case ColorSetId_Light:
                clearClr = clrCreateU32(0xFFEBEBEB);
                txtCont = clrCreateU32(0xFF000000);
                txtDiag = clrCreateU32(0xFFFFFFFF);
                rectLt = clrCreateU32(0xFFDFDFDF);
                rectSh = clrCreateU32(0xFFCACACA);
                tboxClr = clrCreateU32(0xFF505050);
                divClr = clrCreateU32(0xFF000000);
                break;

            default:
            case ColorSetId_Dark:
                //jic
                thmID = ColorSetId_Dark;
                clearClr = clrCreateU32(0xFF2D2D2D);
                txtCont = clrCreateU32(0xFFFFFFFF);
                txtDiag = clrCreateU32(0xFF000000);
                rectLt = clrCreateU32(0xFF505050);
                rectSh = clrCreateU32(0xFF202020);
                tboxClr = clrCreateU32(0xFFEBEBEB);
                divClr = clrCreateU32(0xFFFFFFFF);
                break;
        }
    }

    void init()
    {
        mnuTopLeft = texLoadPNGFile("romfs:/img/fb/menuTopLeft.png");
        mnuTopRight = texLoadPNGFile("romfs:/img/fb/menuTopRight.png");
        mnuBotLeft = texLoadPNGFile("romfs:/img/fb/menuBotLeft.png");
        mnuBotRight = texLoadPNGFile("romfs:/img/fb/menuBotRight.png");
        switch(ui::thmID)
        {
            case ColorSetId_Light:
                //Dark corners
                cornerTopLeft = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerTopLeft.png");
                cornerTopRight = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerTopRight.png");
                cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerBotLeft.png");
                cornerBottomRight = texLoadPNGFile("romfs:/img/tboxDrk/tboxCornerBotRight.png");
                progCovLeft = texLoadPNGFile("romfs:/img/tboxDrk/progBarCoverLeftDrk.png");
                progCovRight = texLoadPNGFile("romfs:/img/tboxDrk/progBarCoverRightDrk.png");

                icn = texLoadPNGFile("romfs:/img/icn/icnDrk.png");
                sideBar = texLoadPNGFile("romfs:/img/fb/lLight.png");
                break;

            default:
                //Light corners
                cornerTopLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopLeft.png");
                cornerTopRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerTopRight.png");
                cornerBottomLeft = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotLeft.png");
                cornerBottomRight = texLoadPNGFile("romfs:/img/tboxLght/tboxCornerBotRight.png");
                progCovLeft = texLoadPNGFile("romfs:/img/tboxLght/progBarCoverLeftLight.png");
                progCovRight = texLoadPNGFile("romfs:/img/tboxLght/progBarCoverRightLight.png");

                icn = texLoadPNGFile("romfs:/img/icn/icnLght.png");
                sideBar = texLoadPNGFile("romfs:/img/fb/lDark.png");
                break;
        }

        top = texCreate(1280, 88);
        bot = texCreate(1280, 72);
        diaBox = texCreate(640, 420);
        texClearColor(diaBox, clrCreateU32(0x00000000));

        if(ui::textMode)
            mstate = TXT_USR;

        loadTrans();
        textUserPrep();

        //Setup top and bottom gfx
        texClearColor(top, clearClr);
        texDraw(icn, top, 66, 27);
        drawText("JKSV", top, shared, 130, 38, 24, ui::txtCont);
        drawRect(top, 30, 87, 1220, 1, ui::txtCont);

        texClearColor(bot, clearClr);
        drawRect(bot, 30, 0, 1220, 1, ui::txtCont);
        drawText(VER_STRING, bot, shared, 8, author.empty() ? 56 : 38, 12, ui::txtCont);
        if(!author.empty())
            drawText(std::string("Translation: " + author).c_str(), bot, ui::shared, 8, 56, 12, ui::txtCont);

        //Not needed anymore
        texDestroy(icn);

        //Setup dialog box
        drawTextbox(diaBox, 0, 0, 640, 420);
        drawRect(diaBox, 0, 56, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));

        util::replaceButtonsInString(userHelp);
        util::replaceButtonsInString(titleHelp);
        util::replaceButtonsInString(folderHelp);
        util::replaceButtonsInString(optHelp);

        //Create graphics to hold guides
        usrGuide = texCreate(textGetWidth(userHelp.c_str(), ui::shared, 18), 28);
        ttlGuide = texCreate(textGetWidth(titleHelp.c_str(), ui::shared, 18), 28);
        fldrGuide = texCreate(textGetWidth(folderHelp.c_str(), ui::shared, 18), 28);
        optGuide = texCreate(textGetWidth(optHelp.c_str(), ui::shared, 18), 28);

        //Clear with bg color
        texClearColor(usrGuide, ui::clearClr);
        texClearColor(ttlGuide, ui::clearClr);
        texClearColor(fldrGuide, ui::clearClr);
        texClearColor(optGuide, ui::clearClr);

        //Draw text to them
        drawText(userHelp.c_str(), usrGuide, ui::shared, 0, 3, 18, ui::txtCont);
        drawText(titleHelp.c_str(), ttlGuide, ui::shared, 0, 3, 18, ui::txtCont);
        drawText(folderHelp.c_str(), fldrGuide, ui::shared, 0, 3, 18, ui::txtCont);
        drawText(optHelp.c_str(), optGuide, ui::shared, 0, 3, 18, ui::txtCont);

        //Calculate x position of help text
        userHelpX = 1220 - usrGuide->width;
        titleHelpX = 1220 - ttlGuide->width;
        folderHelpX = 1220 - fldrGuide->width;
        optHelpX = 1220 - optGuide->width;

        advCopyMenuPrep();
        ui::exMenuPrep();
        ui::optMenuInit();
    }

    void exit()
    {
        texDestroy(cornerTopLeft);
        texDestroy(cornerTopRight);
        texDestroy(cornerBottomLeft);
        texDestroy(cornerBottomRight);
        texDestroy(progCovLeft);
        texDestroy(progCovRight);

        texDestroy(mnuTopLeft);
        texDestroy(mnuTopRight);
        texDestroy(mnuBotLeft);
        texDestroy(mnuBotRight);

        texDestroy(usrGuide);
        texDestroy(ttlGuide);
        texDestroy(fldrGuide);
        texDestroy(optGuide);

        texDestroy(top);
        texDestroy(bot);
        texDestroy(diaBox);

        fontDestroy(shared);
    }

    void drawUI()
    {
        texClearColor(frameBuffer, clearClr);
        texDrawNoAlpha(top, frameBuffer, 0, 0);
        texDrawNoAlpha(bot, frameBuffer, 0, 648);

        switch(mstate)
        {
            case USR_SEL:
                texDrawNoAlpha(usrGuide, frameBuffer, userHelpX, 673);
                break;

            case TTL_SEL:
                texDrawNoAlpha(ttlGuide, frameBuffer, titleHelpX, 673);
                break;

            case FLD_SEL:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                texDrawNoAlpha(fldrGuide, frameBuffer, folderHelpX, 673);
                break;

            case TXT_USR:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                texDrawNoAlpha(usrGuide, frameBuffer, userHelpX, 673);
                break;

            case TXT_TTL:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                texDrawNoAlpha(ttlGuide, frameBuffer, titleHelpX, 673);
                break;

            case TXT_FLD:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                texDrawNoAlpha(fldrGuide, frameBuffer, folderHelpX, 673);
                break;

            case EX_MNU:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                break;

            case OPT_MNU:
                texDrawNoAlpha(sideBar, frameBuffer, 0, 88);
                texDrawNoAlpha(optGuide, frameBuffer, optHelpX, 673);
                break;

            case ADV_MDE:
                drawRect(frameBuffer, 640, 88, 1, 559, ui::txtCont);
                break;
        }
    }

    void drawBoundBox(int x, int y, int w, int h, int clrSh)
    {
        clr rectClr = clrCreateRGBA(0x00, 0x88 + clrSh, 0xC5 + (clrSh / 2), 0xFF);

        texSwapColors(mnuTopLeft, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);
        texSwapColors(mnuTopRight, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);
        texSwapColors(mnuBotLeft, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);
        texSwapColors(mnuBotRight, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF), rectClr);

        switch(ui::thmID)
        {
            case ColorSetId_Light:
                drawRect(frameBuffer, x + 4, y + 4, w - 8, h - 8, clrCreateU32(0xFFFDFDFD));
                break;

            default:
            case ColorSetId_Dark:
                drawRect(frameBuffer, x + 4, y + 4, w - 8, h - 8, clrCreateU32(0xFF212221));
                break;
        }

        //top
        texDraw(mnuTopLeft, frameBuffer, x, y);
        drawRect(frameBuffer, x + 4, y, w - 8, 4, rectClr);
        texDraw(mnuTopRight, frameBuffer, (x + w) - 4, y);

        //mid
        drawRect(frameBuffer, x, y + 4, 4, h - 8, rectClr);
        drawRect(frameBuffer, (x + w) - 4, y + 4, 4, h - 8, rectClr);

        //bottom
        texDraw(mnuBotLeft, frameBuffer, x, (y + h) - 4);
        drawRect(frameBuffer, x + 4, (y + h) - 4, w - 8, 4, rectClr);
        texDraw(mnuBotRight, frameBuffer, (x + w) - 4, (y + h) - 4);

        texSwapColors(mnuTopLeft, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
        texSwapColors(mnuTopRight, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
        texSwapColors(mnuBotLeft, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
        texSwapColors(mnuBotRight, rectClr, clrCreateRGBA(0x00, 0x88, 0xC5, 0xFF));
    }

    void runApp(const uint64_t& down, const uint64_t& held)
    {
        //Draw first. Shouldn't, but it simplifies the showX functions
        drawUI();

        switch(mstate)
        {
            case USR_SEL:
                updateUserMenu(down, held);
                break;

            case TTL_SEL:
                updateTitleMenu(down, held);
                break;

            case FLD_SEL:
                updateFolderMenu(down, held);
                break;

            case ADV_MDE:
                updateAdvMode(down, held);
                break;

            case TXT_USR:
                textUserMenuUpdate(down, held);
                break;

            case TXT_TTL:
                textTitleMenuUpdate(down, held);
                break;

            case TXT_FLD:
                textFolderMenuUpdate(down, held);
                break;

            case EX_MNU:
                updateExMenu(down, held);
                break;

            case OPT_MNU:
                updateOptMenu(down, held);
                break;
        }
        drawPopup(down);
    }
}
