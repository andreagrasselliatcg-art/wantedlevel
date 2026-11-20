#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // Palette "tramonto rosa a Miami"
    // Palette "tramonto pastello": azzurro polvere -> lilla -> rosa -> pesca
    const juce::Colour kNight   { 0xff5a72a6 };   // cielo in alto
    const juce::Colour kPurple  { 0xff8d7bb2 };
    const juce::Colour kMagenta { 0xffcc84a9 };
    const juce::Colour kHorizonC{ 0xfff6a57c };   // pesca all'orizzonte
    const juce::Colour kOrange  { 0xffffaa82 };   // accento pesca
    const juce::Colour kSunTop  { 0xfffff1e2 };
    const juce::Colour kPink    { 0xffff5a96 };
    const juce::Colour kCyan    { 0xffffd6e6 };   // rosa cipria per le etichette
    const juce::Colour kOcean   { 0xff6a4d8a };

    constexpr int kW = 560, kH = 660;
    constexpr float kHorizon = 470.0f;

    juce::Font titleFont (float h)  { return juce::Font (juce::FontOptions (h, juce::Font::bold | juce::Font::italic)); }
    juce::Font boldFont  (float h)  { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }

    float ramp (float x, float a, float b) { return juce::jlimit (0.0f, 1.0f, (x - a) / (b - a)); }

    // Disegna un'immagine @2x centrata su (cx, cy) in coordinate logiche
    void drawCentred (juce::Graphics& g, const juce::Image& img, float cx, float cy)
    {
        if (! img.isValid()) return;
        const float w = (float) img.getWidth() * 0.5f, h = (float) img.getHeight() * 0.5f;
        g.drawImage (img, juce::Rectangle<float> (cx - w * 0.5f, cy - h * 0.5f, w, h));
    }

    juce::Image loadPng (const void* data, int size)
    {
        return juce::ImageCache::getFromMemory (data, size);
    }

    const juce::Colour kPalm { 0xff6e2f5e };   // prugna, come nel riferimento

    // Palma alta e sottile con chioma tonda: fronde in tutte le direzioni che ricadono
    void drawPalm (juce::Graphics& g, float bx, float by, float h, float lean, juce::int64 seed, juce::Colour col)
    {
        using juce::Point;
        juce::Random r (seed);

        auto q  = [] (float t, Point<float> a, Point<float> b, Point<float> c)
                  { return a * ((1 - t) * (1 - t)) + b * (2 * (1 - t) * t) + c * (t * t); };
        auto qd = [] (float t, Point<float> a, Point<float> b, Point<float> c)
                  { return (b - a) * (2 * (1 - t)) + (c - b) * (2 * t); };

        const Point<float> base (bx, by), ctrl (bx + lean * 0.25f, by - h * 0.5f), top (bx + lean, by - h);

        // tronco sottile, allargato alla base e sotto la chioma
        constexpr int N = 40;
        Point<float> L[N + 1], R[N + 1];
        for (int i = 0; i <= N; ++i)
        {
            const float t = (float) i / N;
            const auto  pt = q (t, base, ctrl, top);
            const auto  d  = qd (t, base, ctrl, top);
            const float len = std::hypot (d.x, d.y);
            const Point<float> n (-d.y / len, d.x / len);
            const float w = h * (0.013f - 0.005f * t)
                          + (t < 0.05f ? h * 0.008f * (1.0f - t / 0.05f) : 0.0f)
                          + (t > 0.93f ? h * 0.006f * ((t - 0.93f) / 0.07f) : 0.0f);
            L[i] = pt + n * w;
            R[i] = pt - n * w;
        }
        juce::Path palm;
        palm.startNewSubPath (L[0]);
        for (int i = 1; i <= N; ++i) palm.lineTo (L[i]);
        for (int i = N; i >= 0; --i) palm.lineTo (R[i]);
        palm.closeSubPath();

        g.setColour (col);

        constexpr int numFronds = 19;
        for (int f = 0; f < numFronds; ++f)
        {
            const float deg  = (float) f * (360.0f / numFronds) + r.nextFloat() * 14.0f - 7.0f;
            const float a    = juce::degreesToRadians (deg);
            const float ca   = std::cos (a), sa = std::sin (a);
            const float down = juce::jmax (0.0f, -sa);
            const float fl   = h * (0.27f + 0.08f * r.nextFloat()) * (1.0f - 0.25f * down);

            const Point<float> e (top.x + ca * fl * (0.85f - 0.3f * down),
                                  top.y - sa * fl * 0.7f + fl * (0.38f + 0.25f * (1.0f - std::abs (ca)) * (sa > 0 ? 0.3f : 1.0f)));
            const Point<float> k (top.x + ca * fl * 0.55f,
                                  top.y - sa * fl * 0.55f - fl * 0.22f * (1.0f - down));

            juce::Path rachis;
            rachis.startNewSubPath (top);
            rachis.quadraticTo (k, e);
            g.strokePath (rachis, juce::PathStrokeType (h * 0.0045f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

            for (int j = 1; j <= 22; ++j)
            {
                const float t  = 0.08f + 0.92f * (float) j / 22.0f;
                const auto  pt = q (t, top, k, e);
                const auto  d  = qd (t, top, k, e);
                const float dl = std::hypot (d.x, d.y);
                const Point<float> u (d.x / dl, d.y / dl);
                const float ll = fl * (0.24f * std::pow (std::sin (juce::MathConstants<float>::pi
                                                                  * juce::jmin (t * 1.1f, 1.0f)), 0.5f) + 0.03f)
                                    * (0.8f + 0.4f * r.nextFloat());
                const float bw = fl * 0.016f;

                for (float side : { -1.0f, 1.0f })
                {
                    const float ang = side * (0.6f + 0.3f * r.nextFloat());
                    Point<float> v (u.x * std::cos (ang) - u.y * std::sin (ang),
                                    u.x * std::sin (ang) + u.y * std::cos (ang) + 0.9f);   // foglioline che ricadono
                    v /= std::hypot (v.x, v.y);
                    palm.addTriangle (pt - u * bw, pt + u * bw, pt + v * ll);
                }
            }
        }
        palm.addEllipse (top.x - h * 0.016f, top.y + h * 0.012f - h * 0.022f, h * 0.032f, h * 0.044f);
        g.fillPath (palm);
    }

    // Gabbiano stilizzato
    void drawGull (juce::Graphics& g, float x, float y, float s)
    {
        juce::Path p;
        p.startNewSubPath (x - 10 * s, y - 3 * s);
        p.quadraticTo (x - 5 * s, y - 6 * s, x, y);
        p.quadraticTo (x + 5 * s, y - 7 * s, x + 11 * s, y - 5 * s);
        g.setColour (juce::Colour (0xff4a1f45));
        g.strokePath (p, juce::PathStrokeType (1.8f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

// Fenicottero rosa su una zampa, con i piedi nell'acqua (origine = piedi)
static void drawFlamingo (juce::Graphics& g, float bx, float by, float s)
{
    using namespace juce;
    const auto tf = AffineTransform::scale (s).translated (bx, by);

    // cerchi nell'acqua
    g.setColour (Colour (0xffff8fc8).withAlpha (0.6f));
    g.drawEllipse (bx - 22.0f * s, by - 4.0f * s, 44.0f * s, 8.0f * s, 1.2f);
    g.setColour (Colour (0xffff8fc8).withAlpha (0.3f));
    g.drawEllipse (bx - 38.0f * s, by - 7.0f * s, 76.0f * s, 14.0f * s, 1.2f);

    // zampe: una dritta, una piegata
    Path legs;
    legs.startNewSubPath (0, -128);  legs.lineTo (2, -65);   legs.lineTo (0, 0);
    legs.startNewSubPath (8, -128);  legs.lineTo (18, -92);  legs.lineTo (-8, -102);
    legs.applyTransform (tf);
    g.setColour (Colour (0xffff6fae));
    g.strokePath (legs, PathStrokeType (3.5f * s, PathStrokeType::curved, PathStrokeType::rounded));

    // corpo + coda + collo + testa
    Path bird;
    bird.addEllipse (8 - 44, -150 - 24, 88, 48);
    bird.applyTransform (AffineTransform::rotation (-0.25f, 8, -150));

    Path tail;
    tail.startNewSubPath (38, -166); tail.lineTo (70, -182); tail.lineTo (60, -160); tail.lineTo (44, -144);
    tail.closeSubPath();
    bird.addPath (tail);

    Path neck, neckOutline;
    neck.startNewSubPath (-26, -160);
    neck.cubicTo (-60, -186, -2, -224, -24, -264);
    PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded).createStrokedPath (neckOutline, neck);
    bird.addPath (neckOutline);
    bird.addEllipse (-38, -280, 20, 20);
    bird.applyTransform (tf);

    DropShadow (Colour (0xffff3d96), roundToInt (28.0f * s), {}).drawForPath (g, bird);
    g.setGradientFill (ColourGradient (Colour (0xffffb3d6), 0, by - 185.0f * s,
                                       Colour (0xffff3d8b), 0, by - 125.0f * s, false));
    g.fillPath (bird);

    // ala
    Path wing;
    wing.addEllipse (20 - 26, -153 - 12, 52, 24);
    wing.applyTransform (AffineTransform::rotation (-0.35f, 20, -153).followedBy (tf));
    g.setColour (Colour (0xffbe0f5f).withAlpha (0.5f));
    g.fillPath (wing);

    // becco con punta nera
    Path beak;
    beak.startNewSubPath (-34, -277);
    beak.quadraticTo (-54, -276, -53, -255);
    beak.lineTo (-46, -258);
    beak.quadraticTo (-46, -266, -34, -264);
    beak.closeSubPath();
    beak.applyTransform (tf);
    g.setColour (Colour (0xffffe3f0));
    g.fillPath (beak);

    Path tip;
    tip.startNewSubPath (-53, -263); tip.lineTo (-53, -255); tip.lineTo (-46, -258); tip.lineTo (-47, -264);
    tip.closeSubPath();
    tip.applyTransform (tf);
    g.setColour (Colour (0xff1a0510));
    g.fillPath (tip);

    // occhio
    g.fillEllipse (bx - 32.0f * s, by - 275.0f * s, 4.0f * s, 4.0f * s);
}

// Levetta analogica da joypad vista dall'alto: si inclina verso il valore,
// LED rosa sul bordo come indicatore
static void drawStick (juce::Graphics& g, float cx, float cy, float R, float ang)
{
    using namespace juce;
    const float pi = MathConstants<float>::pi;

    // incavo scuro in cui sta la levetta
    const float wr = R * 1.12f;
    ColourGradient wg (Colour (0xff030204), cx, cy, Colour (0xff1b1520), cx + wr, cy, true);
    wg.addColour (0.6 * R / wr, Colour (0xff030204));
    g.setGradientFill (wg);
    g.fillEllipse (cx - wr, cy - wr, wr * 2, wr * 2);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawEllipse (cx - wr, cy - wr, wr * 2, wr * 2, 1.5f);

    // inclinazione
    const float dx = std::sin (ang), dy = -std::cos (ang);
    const float sx = cx + dx * R * 0.09f, sy = cy + dy * R * 0.09f;

    Path cap;
    cap.addEllipse (sx - R, sy - R, R * 2, R * 2);
    DropShadow (Colours::black.withAlpha (0.8f), 18,
                { roundToInt (-dx * 6 + 3), roundToInt (-dy * 6 + 6) }).drawForPath (g, cap);

    // gomma del bordo
    const float hx = sx - R * 0.3f, hy = sy - R * 0.35f;
    ColourGradient rg (Colour (0xff4a4a52), hx, hy, Colour (0xff161619), hx + R * 1.1f, hy, true);
    rg.addColour (0.7, Colour (0xff2a2a30));
    g.setGradientFill (rg);
    g.fillPath (cap);

    // zigrinatura
    for (int i = 0; i < 64; ++i)
    {
        const float a = (float) i / 64.0f * 2.0f * pi;
        const float c = std::cos (a), sn = std::sin (a);
        const float lit = 0.5f + 0.5f * std::cos (a + 2.3f);
        g.setColour (i % 2 ? Colours::black.withAlpha (0.45f) : Colours::white.withAlpha (0.06f + 0.12f * lit));
        g.drawLine (sx + c * R * 0.82f, sy + sn * R * 0.82f, sx + c * R * 0.97f, sy + sn * R * 0.97f, R * 0.035f);
    }

    Path rimHi;
    rimHi.addCentredArc (sx, sy, R * 0.985f, R * 0.985f, 0.0f, pi * 1.55f, pi * 2.2f, true);
    g.setColour (Colours::white.withAlpha (0.18f));
    g.strokePath (rimHi, PathStrokeType (2.0f));

    // centro concavo (luce da in alto a sinistra -> bordo in basso a destra piu' chiaro)
    const float ir = R * 0.76f;
    ColourGradient dg (Colour (0xff101013), sx - ir, sy - ir, Colour (0xff3a3a41), sx + ir, sy + ir, false);
    dg.addColour (0.55, Colour (0xff232328));
    g.setGradientFill (dg);
    g.fillEllipse (sx - ir, sy - ir, ir * 2, ir * 2);
    g.setColour (Colour (0xff0b0b0d));
    g.drawEllipse (sx - ir, sy - ir, ir * 2, ir * 2, 2.5f);

    Path dishHi;
    dishHi.addCentredArc (sx, sy, ir + 1.5f, ir + 1.5f, 0.0f, pi * 0.55f, pi * 1.25f, true);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.strokePath (dishHi, PathStrokeType (1.2f));

    const float shx = sx - ir * 0.35f, shy = sy - ir * 0.4f;
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.07f), shx, shy,
                                       Colours::white.withAlpha (0.0f), shx + ir * 0.7f, shy, true));
    g.fillEllipse (sx - ir, sy - ir, ir * 2, ir * 2);

    // LED indicatore
    const float lx = sx + dx * R * 0.895f, ly = sy + dy * R * 0.895f;
    const float gr = R * 0.2f;
    g.setGradientFill (ColourGradient (Colour (0xffff3d96).withAlpha (0.7f), lx, ly,
                                       Colour (0xffff3d96).withAlpha (0.0f), lx + gr, ly, true));
    g.fillEllipse (lx - gr, ly - gr, gr * 2, gr * 2);
    g.setColour (Colour (0xffff6fb0));
    g.fillEllipse (lx - R * 0.06f, ly - R * 0.06f, R * 0.12f, R * 0.12f);
    g.setColour (Colours::white);
    g.fillEllipse (lx - R * 0.025f, ly - R * 0.025f, R * 0.05f, R * 0.05f);
}

// =============================================================================
//  LookAndFeel
// =============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,       juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,  kPink.withAlpha (0.4f));
    setColour (juce::Label::textColourId,               juce::Colours::white);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float pos, float startAngle, float endAngle, juce::Slider& s)
{
    const bool big = (bool) s.getProperties().getWithDefault ("big", false);

    auto  bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float size  = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float cx    = bounds.getCentreX(), cy = bounds.getCentreY();
    const float ringR = size * (big ? 0.40f : 0.44f);
    const float bodyR = ringR * 0.74f;
    const float thick = big ? 9.0f : 5.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // traccia di fondo
    juce::Path track;
    track.addCentredArc (cx, cy, ringR, ringR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // arco del valore: glow + colore
    if (pos > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (cx, cy, ringR, ringR, 0.0f, startAngle, angle, true);
        const auto hot = big ? kPink.interpolatedWith (juce::Colours::red, ramp (pos, 0.6f, 1.0f)) : kCyan;

        for (int i = 3; i >= 1; --i)
        {
            g.setColour (hot.withAlpha (0.12f));
            g.strokePath (val, juce::PathStrokeType (thick + (float) i * 6.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }
        g.setGradientFill (juce::ColourGradient (kOrange, cx - ringR, cy + ringR, hot, cx + ringR, cy - ringR, false));
        g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // tacche a stella (solo knob principale): una per ogni livello 0..5
    if (big)
    {
        for (int i = 0; i <= 5; ++i)
        {
            const float a  = startAngle + (float) i / 5.0f * (endAngle - startAngle);
            const float r  = ringR + 22.0f;
            const float sx = cx + r * std::sin (a), sy = cy - r * std::cos (a);
            juce::Path st;
            st.addStar ({ sx, sy }, 5, 3.2f, 7.5f, 0.0f);
            const bool lit = pos * 5.0f >= (float) i - 0.01f && i > 0;
            g.setColour (lit ? juce::Colours::white : juce::Colours::white.withAlpha (0.25f));
            g.fillPath (st);
        }
    }

    // knob principale = levetta analogica
    if (big)
    {
        drawStick (g, cx, cy, bodyR, angle);
        return;
    }

    // corpo del knob piccolo
    juce::ColourGradient body (juce::Colour (0xff5a1a50), cx, cy - bodyR,
                               juce::Colour (0xff140410), cx, cy + bodyR, false);
    g.setGradientFill (body);
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (cx - bodyR * 0.82f, cy - bodyR * 0.82f, bodyR * 1.64f, bodyR * 1.64f, 1.0f);
    g.setColour ((big ? kPink : kCyan).withAlpha (0.8f));
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, big ? 2.0f : 1.4f);

    // indicatore
    const float pw = big ? 6.0f : 3.5f;
    juce::Path ptr;
    ptr.addRoundedRectangle (-pw * 0.5f, -bodyR * 0.88f, pw, bodyR * 0.42f, pw * 0.5f);
    ptr.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (juce::Colours::white);
    g.fillPath (ptr);

}

// =============================================================================
//  Stelle
// =============================================================================
void StarsComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

    const float cell  = b.getWidth() / 5.0f;
    const float outer = juce::jmin (cell, b.getHeight()) * 0.40f;

    for (int i = 0; i < 5; ++i)
    {
        const juce::Point<float> c (b.getX() + cell * ((float) i + 0.5f), b.getCentreY());
        juce::Path star;
        star.addStar (c, 5, outer * 0.46f, outer, 0.0f);

        // contorno scuro spesso, come nell'HUD
        g.setColour (juce::Colours::black);
        g.strokePath (star, juce::PathStrokeType (5.0f, juce::PathStrokeType::mitered));

        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.fillPath (star);

        const float frac = juce::jlimit (0.0f, 1.0f, level - (float) i);
        if (frac > 0.0f && blinkOn)
        {
            const auto sb = star.getBounds();
            g.saveState();
            g.reduceClipRegion (star);
            g.setColour (juce::Colours::white);
            g.fillRect (sb.withWidth (sb.getWidth() * frac));
            g.restoreState();
        }
        else if (frac > 0.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillPath (star);
        }
    }
}

void StarsComponent::mouseDown (const juce::MouseEvent& e)
{
    const int   idx     = juce::jlimit (0, 4, (int) (e.position.x / ((float) getWidth() / 5.0f)));
    const float current = param.convertFrom0to1 (param.getValue());
    float target = (float) (idx + 1);
    if (juce::roundToInt (current) == idx + 1)   // riclic sulla stessa stella = scendi di una
        target = (float) idx;

    param.beginChangeGesture();
    param.setValueNotifyingHost (param.convertTo0to1 (target));
    param.endChangeGesture();
}

// =============================================================================
//  Editor
// =============================================================================
WantedLevelEditor::WantedLevelEditor (WantedLevelProcessor& p)
    : AudioProcessorEditor (&p), proc (p), stars (*p.apvts.getParameter ("level"))
{
    setLookAndFeel (&lnf);

    levelKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    levelKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    levelKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
    levelKnob.getProperties().set ("big", true);
    levelKnob.setMouseDragSensitivity (300);
    addAndMakeVisible (levelKnob);
    levelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "level", levelKnob);

    setupSmallKnob (mixKnob, mixLabel, "MIX");
    setupSmallKnob (outKnob, outLabel, "OUTPUT");
    mixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mix", mixKnob);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "output", outKnob);

    addAndMakeVisible (stars);

    statusImages[0] = loadPng (BinaryData::status_0_png, BinaryData::status_0_pngSize);
    statusImages[1] = loadPng (BinaryData::status_1_png, BinaryData::status_1_pngSize);
    statusImages[2] = loadPng (BinaryData::status_2_png, BinaryData::status_2_pngSize);
    statusImages[3] = loadPng (BinaryData::status_3_png, BinaryData::status_3_pngSize);
    statusImages[4] = loadPng (BinaryData::status_4_png, BinaryData::status_4_pngSize);
    statusImages[5] = loadPng (BinaryData::status_5_png, BinaryData::status_5_pngSize);

    setSize (kW, kH);
    buildBackdrop();
    startTimerHz (30);
}

WantedLevelEditor::~WantedLevelEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void WantedLevelEditor::setupSmallKnob (juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setFont (boldFont (14.0f));
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, kCyan);
    // l'etichetta e' disegnata come immagine Pricedown nello sfondo
}

void WantedLevelEditor::resized()
{
    const int sx = juce::roundToInt (shake.x), sy = juce::roundToInt (shake.y);

    stars.setBounds (kW / 2 - 180 + sx, 112 + sy, 360, 64);
    levelKnob.setBounds (kW / 2 - 145 + sx, 210 + sy, 290, 290);

    mixKnob.setBounds (70, 540, 100, 100);
    mixLabel.setBounds (70, 520, 100, 20);
    outKnob.setBounds (kW - 170, 540, 100, 100);
    outLabel.setBounds (kW - 170, 520, 100, 20);
}

void WantedLevelEditor::buildBackdrop()
{
    const float scale = 2.0f;
    backdrop = juce::Image (juce::Image::ARGB, (int) (kW * scale), (int) (kH * scale), true);
    juce::Graphics g (backdrop);
    g.addTransform (juce::AffineTransform::scale (scale));

    const float W = (float) kW, H = (float) kH;

    // --- cielo al tramonto -------------------------------------------------
    juce::ColourGradient sky (kNight, 0, 0, kHorizonC, 0, kHorizon, false);
    sky.addColour (0.40, kPurple);
    sky.addColour (0.70, kMagenta);
    g.setGradientFill (sky);
    g.fillRect (0.0f, 0.0f, W, kHorizon);

    // stelline nel cielo
    juce::Random rnd (1986);
    for (int i = 0; i < 70; ++i)
    {
        const float x = rnd.nextFloat() * W, y = rnd.nextFloat() * 220.0f;
        g.setColour (juce::Colours::white.withAlpha (0.15f + rnd.nextFloat() * 0.45f));
        g.fillEllipse (x, y, 1.6f, 1.6f);
    }

    // --- sole a strisce ----------------------------------------------------
    const float sunR = 170.0f, sunCx = W * 0.5f, sunCy = kHorizon - 40.0f;
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<int> (0, 0, kW, (int) kHorizon));
        for (int k = 0; k < 7; ++k)
        {
            const float yy = sunCy - 10.0f + (float) k * 17.0f;
            g.excludeClipRegion (juce::Rectangle<float> (0.0f, yy, W, 2.0f + (float) k * 1.6f).toNearestInt());
        }
        juce::ColourGradient sun (kSunTop, sunCx, sunCy - sunR, juce::Colour (0xfff27ba0), sunCx, sunCy + sunR * 0.4f, false);
        sun.addColour (0.5, juce::Colour (0xffffc39c));
        g.setGradientFill (sun);
        g.fillEllipse (sunCx - sunR, sunCy - sunR, sunR * 2.0f, sunR * 2.0f);
    }
    // alone del sole
    g.setGradientFill (juce::ColourGradient (kOrange.withAlpha (0.35f), sunCx, sunCy,
                                             kOrange.withAlpha (0.0f), sunCx + sunR * 1.8f, sunCy, true));
    g.fillEllipse (sunCx - sunR * 1.8f, sunCy - sunR * 1.8f, sunR * 3.6f, sunR * 3.6f);

    // --- skyline sull'orizzonte -------------------------------------------
    {
        juce::Random r2 (2026);
        float x = 0.0f;
        while (x < W)
        {
            const float bw = 14.0f + r2.nextFloat() * 26.0f;
            const float centreBoost = 1.0f - std::abs ((x - W * 0.5f) / (W * 0.5f));
            const float bh = 18.0f + r2.nextFloat() * 50.0f + centreBoost * 45.0f;
            g.setColour (juce::Colour (0xff7a4a7e));
            g.fillRect (x, kHorizon - bh, bw, bh);

            // finestre illuminate
            for (float wy = kHorizon - bh + 5.0f; wy < kHorizon - 4.0f; wy += 7.0f)
                for (float wx = x + 3.0f; wx < x + bw - 3.0f; wx += 6.0f)
                    if (r2.nextFloat() < 0.22f)
                    {
                        g.setColour ((r2.nextBool() ? kSunTop : kCyan).withAlpha (0.55f));
                        g.fillRect (wx, wy, 2.0f, 2.5f);
                    }
            x += bw + 1.0f;
        }
    }

    // --- oceano ------------------------------------------------------------
    juce::ColourGradient ocean (kOcean, 0, kHorizon, juce::Colour (0xff1a0f26), 0, H, false);
    ocean.addColour (0.35, juce::Colour (0xff3e2858));
    g.setGradientFill (ocean);
    g.fillRect (0.0f, kHorizon, W, H - kHorizon);

    // riflesso del sole sull'acqua
    for (int k = 0; k < 14; ++k)
    {
        const float yy = kHorizon + 6.0f + (float) k * 9.5f;
        const float hw = 120.0f - (float) k * 6.0f;
        g.setColour (kPink.interpolatedWith (kOrange, (float) k / 14.0f).withAlpha (0.5f - (float) k * 0.03f));
        g.fillRoundedRectangle (sunCx - hw, yy, hw * 2.0f, 2.2f, 1.0f);
    }
    g.setColour (juce::Colour (0xffffbea0).withAlpha (0.7f));
    g.drawHorizontalLine ((int) kHorizon, 0.0f, W);

    // --- palme -------------------------------------------------------------
    // palme lontane (piu' chiare, sfumate nella foschia)
    drawPalm (g, 150.0f, kHorizon + 5.0f,  215.0f,  -8.0f, 61, juce::Colour (0xff9c6aa2));
    drawPalm (g, 410.0f, kHorizon + 5.0f,  240.0f,  10.0f, 73, juce::Colour (0xff9c6aa2));
    drawPalm (g,  18.0f, kHorizon + 5.0f,  370.0f,  14.0f, 91, juce::Colour (0xff8a5a95));
    drawPalm (g, 470.0f, kHorizon + 5.0f,  330.0f,  22.0f, 83, juce::Colour (0xff8a5a95));
    // palme vicine
    drawPalm (g,  45.0f, kHorizon + 50.0f, 330.0f,  -6.0f, 11, kPalm);
    drawPalm (g, 535.0f, kHorizon + 50.0f, 350.0f, -10.0f, 37, kPalm);

    // --- gabbiani --------------------------------------------------------------
    drawGull (g, 215.0f, 250.0f, 1.5f);
    drawGull (g, 248.0f, 268.0f, 1.1f);
    drawGull (g, 196.0f, 282.0f, 1.25f);

    // --- fenicottero rosa ----------------------------------------------------
    drawFlamingo (g, 488.0f, kHorizon + 36.0f, 0.95f);

    // --- titolo e scritte (font Pricedown pre-renderizzato) ---------------
    drawCentred (g, loadPng (BinaryData::title_png,    BinaryData::title_pngSize),    W * 0.5f, 50.0f);
    drawCentred (g, loadPng (BinaryData::subtitle_png, BinaryData::subtitle_pngSize), W * 0.5f, 99.0f);
    drawCentred (g, loadPng (BinaryData::label_mix_png,    BinaryData::label_mix_pngSize),    120.0f,     530.0f);
    drawCentred (g, loadPng (BinaryData::label_output_png, BinaryData::label_output_pngSize), W - 120.0f, 530.0f);

    // footer
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (boldFont (11.0f));
    g.drawText ("TUNE TALKS  -  v1.0", juce::Rectangle<float> (0.0f, H - 22.0f, W, 16.0f),
                juce::Justification::centred);
}

void WantedLevelEditor::paint (juce::Graphics& g)
{
    g.drawImage (backdrop, getLocalBounds().toFloat());

    const float W = (float) kW;
    const float heat = ramp (level, 3.0f, 5.0f);

    // --- luci della polizia (da 3 stelle in su) -------------------------
    if (heat > 0.0f)
    {
        const bool phaseA = std::fmod (animTime * (2.0f + 4.0f * heat), 1.0f) < 0.5f;
        const auto red  = juce::Colour (0xffff1a3c);
        const auto blue = juce::Colour (0xff2a6bff);
        const auto left  = phaseA ? red : blue;
        const auto right = phaseA ? blue : red;
        const float r = 160.0f + 120.0f * heat, a = 0.55f * heat;

        g.setGradientFill (juce::ColourGradient (left.withAlpha (a), 0, 0, left.withAlpha (0.0f), r, 0, true));
        g.fillEllipse (-r, -r, r * 2.0f, r * 2.0f);
        g.setGradientFill (juce::ColourGradient (right.withAlpha (a), W, 0, right.withAlpha (0.0f), W - r, 0, true));
        g.fillEllipse (W - r, -r, r * 2.0f, r * 2.0f);
    }

    // --- alone dietro il knob, pulsa con l'audio ----------------------------
    {
        const auto kb = levelKnob.getBounds().toFloat();
        const float glow = juce::jlimit (0.0f, 1.0f, meter) * (0.25f + 0.75f * ramp (level, 0.0f, 5.0f));
        const auto col = kPink.interpolatedWith (juce::Colours::red, heat);
        const float gr = kb.getWidth() * (0.45f + 0.15f * glow);
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f * glow), kb.getCentreX(), kb.getCentreY(),
                                                 col.withAlpha (0.0f), kb.getCentreX() + gr, kb.getCentreY(), true));
        g.fillEllipse (kb.getCentreX() - gr, kb.getCentreY() - gr, gr * 2.0f, gr * 2.0f);
    }

    // --- stato del crimine ---------------------------------------------------
    drawCentred (g, statusImages[juce::jlimit (0, 5, juce::roundToInt (level))], W * 0.5f, 195.0f);
}

void WantedLevelEditor::timerCallback()
{
    animTime += 1.0f / 30.0f;
    level = proc.apvts.getRawParameterValue ("level")->load();
    meter = proc.outputMeter.load();

    stars.level   = level;
    stars.blinkOn = level < 4.5f || std::fmod (animTime * 3.0f, 1.0f) < 0.6f;   // lampeggiano a 5 stelle

    // tremolio dell'interfaccia a 5 stelle
    const float s = ramp (level, 4.6f, 5.0f) * juce::jlimit (0.0f, 1.0f, meter) * 3.0f;
    auto& r = juce::Random::getSystemRandom();
    const juce::Point<float> newShake (s > 0.0f ? (r.nextFloat() - 0.5f) * 2.0f * s : 0.0f,
                                       s > 0.0f ? (r.nextFloat() - 0.5f) * 2.0f * s : 0.0f);
    if (newShake != shake) { shake = newShake; resized(); }

    stars.repaint();
    repaint();
}
