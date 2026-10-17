#pragma once
#include <juce_core/juce_core.h>

// Hints and tour texts in English and Russian. The interface itself is always in English.
namespace Help
{
    enum class Lang { en, ru };

    struct Entry { const char* key; const char* en; const char* ru; };

    inline const Entry tips[] = {
        { "input", "Input gain. A hotter voice drives the vocoder and the dirt harder.",
                   "Входная громкость. Чем громче голос, тем сильнее работают вокодер и грязь." },
        { "output", "Output gain, applied before Clip.", "Выходная громкость, до клиппера Clip." },
        { "inMeter", "Input level. The number on top is the peak; click it to reset.", "Уровень на входе. Сверху пик, клик по нему сбрасывает." },
        { "outMeter", "Output level. The peak turns red above 0 dB; click it to reset.", "Уровень на выходе. Пик краснеет выше 0 dB, клик по нему сбрасывает." },
        { "presetName", "Presets. Click for the list: factory presets, your presets, rename, delete and the folder. * means unsaved changes.",
                        "Пресеты. Клик: заводские и твои пресеты, переименование, удаление и папка. * значит, есть несохранённые изменения." },
        { "prev", "Previous preset.", "Предыдущий пресет." },
        { "next", "Next preset.", "Следующий пресет." },
        { "save", "Save the current settings as a preset.", "Сохранить текущие настройки как пресет." },
        { "gear", "Theme, window size, help and the tour.", "Тема, размер окна, подсказки и обучение." },
        { "logo", "Version info.", "Информация о версии." },
        { "view", "Screen: WAVE shows the output, SPECTRUM the vocoder bands, EQ the filter curves.",
                  "Экран: WAVE показывает выход, SPECTRUM полосы вокодера, EQ кривые фильтров." },
        { "scopeWave", "Mouse wheel: zoom the height. Shift + wheel: how many bars are shown. Double-click: reset.",
                       "Колёсико: масштаб по высоте. Shift + колёсико: сколько тактов на экране. Двойной клик: сброс." },
        { "scopeSpec", "Every bar is one vocoder band, lit by your voice. More Bands means more bars.",
                       "Каждый столбик это полоса вокодера, её зажигает твой голос. Больше Bands, больше столбиков." },
        { "scopeEq", "Drag the points: Low Cut, peak and High Cut. Wheel on a cut point changes its slope, on the peak its width. Double-click a point to reset it.",
                     "Тяни точки: Low Cut, пик и High Cut. Колёсико на точке среза меняет крутизну, на пике ширину. Двойной клик по точке сбрасывает её." },
        { "prePill", "PRE EQ: filters only the voice that goes into the effect. The clean voice for Mix stays untouched. Light: on/off, name: edit.",
                     "PRE EQ: фильтрует только голос, который идёт в эффект. Чистый голос для Mix не трогается. Огонёк: вкл/выкл, название: настройка." },
        { "postPill", "POST EQ: filters only the effect. With Mix below 100 % the clean voice keeps its full range. Light: on/off, name: edit.",
                      "POST EQ: фильтрует только эффект. Если Mix меньше 100 %, чистый голос остаётся полным. Огонёк: вкл/выкл, название: настройка." },
        { "noteTag", "Where the robot's pitch comes from right now.", "Откуда сейчас берётся нота робота." },
        { "power", "Switch this block on or off.", "Включить или выключить блок." },
        { "vocPower", "Vocoder off: the voice itself goes through the dirt, and Harmonize pitch-shifts it.",
                      "Вокодер выключен: через грязь идёт сам голос, а Harmonize сдвигает его по высоте." },

        { "source", "What the robot is made of: Saw, Square, Supersaw, Noise (whisper), Sidechain (audio from another track) or Synth + SC.",
                    "Из чего сделан робот: пила, квадрат, суперпила, шум (шёпот), сайдчейн (звук с другой дорожки) или синт + сайдчейн." },
        { "pitch", "MIDI: notes from another track (Bitwig: Note Receiver). Auto: follows the melody of your voice. Fixed: one note.",
                   "MIDI: ноты с другой дорожки (в Bitwig через Note Receiver). Auto: повторяет мелодию голоса. Fixed: одна нота." },
        { "note", "The note for Fixed mode.", "Нота для режима Fixed." },
        { "glide", "How smoothly the robot slides between notes.", "Насколько плавно робот переходит между нотами." },
        { "detune", "Detunes the oscillators against each other: fatter and wider.", "Расстройка генераторов: толще и шире." },
        { "spread", "How wide the oscillators are spread in stereo.", "Насколько широко генераторы разведены по стерео." },
        { "noise", "Adds noise to the carrier so consonants cut through.", "Подмешивает шум в несущую, чтобы согласные читались." },

        { "bands", "Number of vocoder bands. Few: blocky 80s robot. Many: clear speech.",
                   "Количество полос вокодера. Мало: угловатый робот 80-х. Много: разборчивая речь." },
        { "formant", "Voice size. Down: a giant demon. Up: a small goblin. The melody stays.",
                     "Размер голоса. Вниз: огромный демон. Вверх: маленький гоблин. Мелодия не меняется." },
        { "attack", "How fast the bands follow the voice.", "Как быстро полосы реагируют на голос." },
        { "release", "How fast the bands fade. Long values smear the robot into a pad.", "Как быстро полосы затухают. Длинные значения размазывают робота в пэд." },
        { "sibilance", "Brings back s, sh, t from the original voice so the words stay readable.",
                       "Возвращает с, ш, т из оригинального голоса, чтобы слова читались." },
        { "gate", "Mutes the noise between phrases. Everything quieter than this is silenced.",
                  "Глушит шум между фразами. Всё тише этого уровня замолкает." },

        { "harmPower", "Harmonize on or off.", "Включить или выключить Harmonize." },
        { "chip", "Switch this harmony voice on or off. A dot means the scale moved the interval to the nearest note.",
                  "Включить или выключить голос гармонии. Точка значит, что лад сдвинул интервал к ближайшей ноте." },
        { "fader", "Level of this harmony voice. Double-click resets it.", "Громкость этого голоса гармонии. Двойной клик сбрасывает." },
        { "panDot", "Drag the dot to place this voice in stereo (Manual mode). Double-click centres it.",
                    "Тяни точку, чтобы поставить голос в стерео (режим Manual). Двойной клик ставит в центр." },
        { "scaleOn", "Scale on: thirds, fourths and fifths follow the chosen scale. Off: exact intervals.",
                     "Лад включён: терции, кварты и квинты подстраиваются под лад. Выключен: точные интервалы." },
        { "root", "Root note of the scale.", "Тоника лада." },
        { "scale", "The scale the harmonies follow.", "Лад, под который подстраиваются гармонии." },
        { "panMode", "Manual: place every voice with its dot. Spectral: each voice is split into bands that flicker across the stereo field.",
                     "Manual: каждый голос ставишь точкой. Spectral: голоса режутся на полосы, которые мерцают по стерео." },
        { "alt", "Quick layout: voices alternate left and right.", "Быстрая расстановка: голоса по очереди влево и вправо." },
        { "fan", "Quick layout: low voices left, high voices right.", "Быстрая расстановка: низкие голоса левее, высокие правее." },
        { "harmLevel", "Overall level of all harmonies.", "Общая громкость всех гармоний." },
        { "harmDetune", "Detunes the harmony voices for a choir effect.", "Расстройка голосов гармонии, эффект хора." },
        { "harmSpread", "Pushes every voice towards its side. Voices already at the edge stay there.",
                        "Толкает каждый голос к своему краю. Голоса, которые уже у края, там и остаются." },

        { "dirtMode", "Saturation character: Soft, Tape, Tube, Fuzz, Hard or Fold.", "Характер насыщения: Soft, Tape, Tube, Fuzz, Hard или Fold." },
        { "drive", "Overdrives the carrier before the vocoder: fatter, angrier robot.", "Перегружает несущую до вокодера: робот толще и злее." },
        { "crunch", "Saturation inside every band: the main crunch.", "Насыщение внутри каждой полосы: главный хруст." },
        { "destroy", "Bitcrush and downsample after the vocoder.", "Биткраш и понижение частоты после вокодера." },
        { "compMode", "OTT: multiband up and down compression, pulls the grit in your face. Glue: smooth compressor.",
                      "OTT: многополосная компрессия вверх и вниз, вытаскивает грязь в лицо. Glue: мягкий компрессор." },
        { "ottDepth", "Overall amount of OTT.", "Общая сила OTT." },
        { "ottTime", "How fast OTT reacts. Left fast, right slow.", "Скорость реакции. Влево быстро, вправо медленно." },
        { "ottUp", "How much the quiet parts are pulled up.", "Насколько поднимаются тихие места." },
        { "ottDown", "How much the loud parts are pushed down.", "Насколько придавливаются громкие места." },
        { "glueThresh", "Level where the compression starts.", "Уровень, с которого начинается сжатие." },
        { "glueRatio", "How hard everything above the threshold is squeezed.", "Насколько сильно сжимается всё выше порога." },
        { "glueAttack", "How fast the compression grabs.", "Как быстро срабатывает сжатие." },
        { "glueRelease", "How fast the compression lets go.", "Как быстро сжатие отпускает." },
        { "compMix", "Blend of compressed and uncompressed effect.", "Смесь сжатого и несжатого эффекта." },
        { "widthMode", "Spectral: bands split left and right, mono safe. Haas: a short delay on one side. Chorus: moving copies.",
                       "Spectral: полосы разводятся влево и вправо, безопасно в моно. Haas: короткая задержка одной стороны. Chorus: плавающие копии." },
        { "width", "How wide the effect is.", "Ширина эффекта." },
        { "monoBass", "Everything below this frequency stays in mono.", "Всё ниже этой частоты остаётся в моно." },
        { "autoGain", "Keeps the effect as loud as the clean voice.", "Держит громкость эффекта на уровне чистого голоса." },
        { "mix", "Clean voice vs effect. 0: clean, 100: full effect.", "Чистый голос и эффект. 0: чисто, 100: эффект на полную." },
        { "clipMode", "Soft rounds the peaks off, Hard cuts them flat.", "Soft скругляет пики, Hard срезает ровно." },
        { "ceiling", "Nothing goes above this level. Turn it to see the red lines on the screen.",
                     "Выше этого уровня ничего не пройдёт. Покрути, и на экране появятся красные линии." },
    };

    inline juce::String tip (const juce::String& key, Lang lang)
    {
        for (const auto& e : tips)
            if (key == e.key)
                return juce::String::fromUTF8 (lang == Lang::ru ? e.ru : e.en);
        return {};
    }

    struct Step { const char* target; const char* titleEn; const char* textEn; const char* titleRu; const char* textRu; };

    inline const Step tour[] = {
        { "", "Welcome to Voice Fucker",
              "A quick tour of every part. Use Next and Back, or Skip at any time. You can start it again from the gear menu.",
              "Добро пожаловать в Voice Fucker",
              "Короткий тур по всем частям. Листай кнопками «Дальше» и «Назад» или пропусти в любой момент. Запустить заново можно из меню шестерёнки." },
        { "inCol", "Input", "Put the plugin on your vocal track. Input gain and level are here.",
                   "Вход", "Ставь плагин на дорожку с вокалом. Здесь входная громкость и уровень." },
        { "presets", "Presets", "Arrows to browse, the name for the list, the disk to save. Factory presets come first.",
                     "Пресеты", "Стрелками листать, по названию открывается список, дискетка сохраняет. Сначала идут заводские пресеты." },
        { "gear", "Settings", "Theme, window size, help on or off, help language and this tour.",
                  "Настройки", "Тема, размер окна, подсказки, язык подсказок и этот тур." },
        { "scope", "The screen", "WAVE shows the output, SPECTRUM the vocoder bands, EQ the filter curves.",
                   "Экран", "WAVE показывает выход, SPECTRUM полосы вокодера, EQ кривые фильтров." },
        { "pills", "PRE and POST EQ",
                   "PRE filters only the voice that feeds the effect. POST filters only the effect. The clean voice for Mix is never filtered.",
                   "PRE и POST EQ",
                   "PRE фильтрует только голос, который идёт в эффект. POST фильтрует только эффект. Чистый голос для Mix не фильтруется никогда." },
        { "noteTag", "Pitch readout", "Shows where the robot gets its notes: MIDI notes, the note of your voice (Auto) or the fixed note.",
                     "Нота", "Показывает, откуда робот берёт ноты: MIDI, нота твоего голоса (Auto) или фиксированная нота." },
        { "carrier", "Carrier", "What the robot is made of and where its pitch comes from. Pitch Auto needs nothing: the robot sings your melody.",
                     "Carrier", "Из чего сделан робот и откуда берётся его нота. Pitch Auto работает сразу: робот поёт твою мелодию." },
        { "midi", "MIDI and sidechain",
                  "MIDI: in Bitwig put a Note Receiver before the plugin and pick the notes track. Sidechain: choose Source Sidechain and route a synth into the plugin's sidechain input.",
                  "MIDI и сайдчейн",
                  "MIDI: в Bitwig поставь перед плагином Note Receiver и выбери дорожку с нотами. Сайдчейн: выбери Source Sidechain и заведи синт во вход сайдчейна плагина." },
        { "vocoder", "Vocoder", "Bands sets how robotic it is, Formant the size of the voice, Sibilance keeps the words readable, Gate cleans the pauses.",
                     "Vocoder", "Bands задаёт, насколько звук роботизированный, Formant размер голоса, Sibilance разборчивость слов, Gate чистит паузы." },
        { "harmonize", "Harmonize",
                       "Extra voices from -24 to +12. Each has its own fader and stereo dot. With the vocoder off, the voice itself is pitch-shifted.",
                       "Harmonize",
                       "Дополнительные голоса от -24 до +12. У каждого свой фейдер и точка в стерео. Если вокодер выключен, сдвигается по высоте сам голос." },
        { "harmScale", "Root and Scale", "Thirds, fourths and fifths follow the scale. The dot on a button shows the interval was moved.",
                       "Root и Scale", "Терции, кварты и квинты подстраиваются под лад. Точка на кнопке показывает, что интервал сдвинут." },
        { "dirt", "Dirt", "Drive heats the carrier, Crunch rips every band, Destroy crushes the result. Six characters in Mode.",
                  "Dirt", "Drive греет несущую, Crunch рвёт каждую полосу, Destroy крошит результат. Шесть характеров в Mode." },
        { "comp", "Comp", "OTT pulls the grit in your face, Glue holds it together.",
                  "Comp", "OTT вытаскивает грязь в лицо, Glue держит всё вместе." },
        { "width", "Width", "Spectral is mono safe, Haas and Chorus are extra wide. Mono bass keeps the low end centred.",
                   "Width", "Spectral безопасен в моно, Haas и Chorus дают сверхширину. Mono bass держит низ по центру." },
        { "mix", "Mix", "How much effect you hear. Auto gain keeps the level steady.",
                 "Mix", "Сколько эффекта слышно. Auto gain держит громкость ровной." },
        { "clip", "Clip", "The last stage: nothing goes above Ceiling. Turn Ceiling to see the red lines on the screen.",
                  "Clip", "Последний этап: выше Ceiling ничего не пройдёт. Покрути Ceiling, и на экране появятся красные линии." },
        { "outCol", "Output", "Output gain and level. The peak turns red above 0 dB.",
                    "Выход", "Выходная громкость и уровень. Пик краснеет выше 0 dB." },
        { "", "That's it!", "Hover over anything for a hint. Go wreck some vocals.",
              "Готово!", "Наведи мышку на что угодно, и появится подсказка. Иди ломать вокал." },
    };
    inline constexpr int tourLength = (int) (sizeof (tour) / sizeof (tour[0]));

    inline juce::String ui (const char* en, const char* ru, Lang lang)
    {
        return juce::String::fromUTF8 (lang == Lang::ru ? ru : en);
    }
}
