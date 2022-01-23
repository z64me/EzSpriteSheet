#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QColorDialog>
#include <QColor>
#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <math.h>

#include "ezspritesheet.h"

static bool SettingsChanged = false;
static MainWindow *global = 0;

#define append(X, ...) sprintf((X) + strlen(X), __VA_ARGS__)
#define OFFSTR "off"
#define ONSTR "on"
#define BOOL_ON_OFF(X) (X) != 0 ? ONSTR : OFFSTR

static void die_overload(const char *msg)
{
    QMessageBox::critical(global, "Fatal Error", msg);
}

static void complain_overload(const char *msg)
{
    QMessageBox::warning(global, "Warning", msg);
}

static void success_overload(const char *msg)
{
    QMessageBox::information(global, "Success", msg);
}

static void progress(float unit_interval)
{
    unit_interval *= 100;
    if (unit_interval >= 100)
        unit_interval = 100;
    global->ui->progressBar->setValue(unit_interval);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    /*QImage *img = new QImage(256, 256, QImage::Format_RGBA8888);

    for (int i = 0; i < img->width(); ++i)
    {
        for (int k = 0; k < img->height(); ++k)
        {
            img->setPixel(i, k, qRgba(i * k, i * k, i *k, 255));
        }
    }*/

    this->settings.pivot.setRgb(0, 255, 0);
    ui->setupUi(this);
    resize(600, 10); /* shrink window to minimum size */
    PropagatePageCombo(0);
    EzSpriteSheet_setPopups(die_overload, complain_overload, success_overload);
    //ui->imagePreview->setPixmap(QPixmap::fromImage(*img));
   // ui->imagePreview->adjustSize();
    global = this;
}

void MainWindow::SettingsChange(void)
{
    if (this->ui->selectFolderString->isEnabled() == false)
        return;
    this->ui->buttonRefresh->setEnabled(true);
    SettingsChanged = true;
}

void MainWindow::PropagatePageCombo(int num)
{
    QComboBox *pageCombo = this->ui->pageCombo;

    /* disable if no pages */
    pageCombo->setEnabled(num != 0);
    pageCombo->clear();

    if (!num)
    {
        ui->pageStats->setText("");
        ui->pageStatsHeader->setVisible(false);
        ui->globalStats->setText("");
        ui->globalStatsHeader->setVisible(false);
    }

    for (int i = 0; i < num; ++i)
    {
        char buf[256];

        snprintf(buf, sizeof(buf), "%d / %d", i + 1, num);
        pageCombo->addItem(buf);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setFolderText(QString str)
{
    this->ui->selectFolderString->setText(str);
    this->ui->selectFolderString->setEnabled(true);
    this->ui->buttonExport->setEnabled(false);
    SettingsChange();

    ui->imagePreview->setText("Click the 'Refresh' button to\ngenerate new sprite sheets.");
}

void MainWindow::setExistingDirectory()
{
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;//QFileDialog::DontUseNativeDialog;
    options |= QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory = QFileDialog::getExistingDirectory(this,
                                tr("Select the directory containing game sprites"),
                                this->ui->selectFolderString->text(),// directoryLabel->text(),
                                options);
    if (!directory.isEmpty())
        setFolderText(directory);
}

void MainWindow::on_selectFolderButton_clicked()
{
    setExistingDirectory();
    //QDebug("what") << "what";
}

void MainWindow::on_widthValue_valueChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_heightValue_valueChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_methodValue_currentIndexChanged(int index)
{
    (void)index;
    SettingsChange();
}

void MainWindow::on_cbTrim_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbDoubles_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbRotate_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbPivot_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbPad_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_valuePad_valueChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_8_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_9_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_10_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_12_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_7_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_cbExhaust_6_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

int MainWindow::on_buttonRefresh_clicked()
{
    const char *msg;
    char formats[256] = {0};
    int width = this->ui->widthValue->value();
    int height = this->ui->heightValue->value();
    int visual = this->ui->cbExhaust_6->isChecked();
    int doubles = this->ui->cbDoubles->isChecked();
    int exhaustive = this->ui->cbExhaust->isChecked();
    int rotate = this->ui->cbRotate->isChecked();
    int trim = this->ui->cbTrim->isChecked();
    int pad = this->ui->valuePad->value();
    int negate = this->ui->cbExhaust_7->isChecked();
    uint32_t color = (this->settings.pivot.red() << 16) | (this->settings.pivot.green() << 8) | this->settings.pivot.blue();

    if (this->ui->cbPad->isChecked() == false)
        pad = 0;

    if (this->ui->cbPivot->isChecked() == false)
        color = 0;

    /* image formats */
    if (this->ui->cbExhaust_8->isChecked())
        append(formats, "png,");
    if (this->ui->cbExhaust_9->isChecked())
        append(formats, "gif,");
    if (this->ui->cbExhaust_10->isChecked())
        append(formats, "webp,");
    if (!strlen(formats))
    {
        QMessageBox::warning(this,
                             "No image formats enabled",
                             "Please ensure at least one image format is enabled.");
        return 1;
    }

    this->ui->buttonRefresh->setEnabled(false);
    PropagatePageCombo(0);

    /* throw the retrieved arguments at the main driver */
    msg = EzSpriteSheet(
        formats
        , this->ui->cbExhaust_12->isChecked() ? this->ui->textRegex->text().toUtf8().constData() : 0
        , this->ui->methodValue->currentText().toUtf8().constData()
        , 0 /* scheme, unused during this step */
        , this->ui->selectFolderString->text().toUtf8().constData()
        , 0 /* output, unused during this step */
        , 0 /* logfile, unused in gui */
        , 0 /* warnings, unused in gui */
        , 0 /* quiet, unused in gui */
        , exhaustive
        , rotate
        , trim
        , doubles
        , pad
        , visual
        , width
        , height
        , negate
        , color
        , &stats.sprites
        , &stats.duplicates
        , progress /* pack_progress */
        , progress /* load_progress */
    );

    SettingsChanged = false;
    this->ui->buttonRefresh->setEnabled(false);

    /* something went wrong */
    if (msg)
    {
        this->ui->buttonExport->setEnabled(false);
        ui->imagePreview->setText(msg);
        return 1;
    }

    this->ui->buttonExport->setEnabled(true);

    PropagatePageCombo(EzSpriteSheet_countPages());

    return 0;
}

void MainWindow::on_buttonAbout_clicked()
{
    QMessageBox *box;

    box = new QMessageBox(QMessageBox::Information,
                          "About EzSpriteSheet",
                          "EzSpriteSheet v1.0.0<br/><br/>"
                          "by z64me<br/><br/>"
                          "Updates, support, license, and source code are available on my website:<br/><br/>"
                          "<a href=\"https://z64.me\">https://z64.me/</a>",
                          QMessageBox::Ok,
                          this);
    box->setTextFormat(Qt::RichText);
   // box->setStyleSheet("QLabel{min-width:320px;min-height:120px;}");
    box->exec();
    delete box;
}

void MainWindow::on_buttonExport_clicked()
{
    const char *errstr;

    if (SettingsChanged)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,
                                  "Settings have changed.",
                                  "Settings have changed. Reprocess with new settings before exporting?",
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                  QMessageBox::Cancel);
        if (reply != QMessageBox::No && reply != QMessageBox::Yes)
            return;

        if (reply == QMessageBox::Yes)
        {
            if (on_buttonRefresh_clicked())
                return;
        }
    }

    static QLabel prev;
    static QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this,
                                "Save sprite bank",
                                prev.text(),
                                "JSON (.json)(*.json);;XML (.xml)(*.xml);;C99 Header (.h)(*.h)",
                                &selectedFilter);
    if (fileName.isEmpty())
        return;
    prev.setText(fileName);

    /* export */
    if ((errstr = EzSpriteSheet_export(
             prev.text().toUtf8().constData()
             , selectedFilter.toUtf8().constData()
             , this->ui->cbPrefix->isChecked() ? this->ui->textPrefix->text().toUtf8().constData() : 0
             , this->ui->cbLongname->isChecked()
             , progress/*progress_export*/)
    ))
    {
        QMessageBox::critical(this, "Fatal Error", errstr);
    }
}

void MainWindow::on_cbPrefix_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_textPrefix_textChanged(const QString &arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_buttonPivot_clicked()
{
    char magic[64];
    QColor c;

    c = QColorDialog::getColor(this->settings.pivot);

    /* only accept non-zero color (zero is used specify pivot = off) */
    if (c.red() == 0 && c.green() == 0 && c.blue() == 0)
        return;

    this->settings.pivot = c;

    sprintf(magic, "background-color: #%02x%02x%02x;", c.red(), c.green(), c.blue());
    ui->buttonPivot->setStyleSheet(magic);
}

void MainWindow::on_pageCombo_currentIndexChanged(int index)
{
    if (index < 0)
        return;

    char buf[512];
    QImage *img;
    int w;
    int h;
    int sprites;
    float occupancy;
    uint8_t *pix = (uint8_t*)EzSpriteSheet_getPagePixels(index, &w, &h, &sprites, &occupancy, 0);

    if (!pix)
        return;

    /* generate preview */
    float Rw = fmin(float(w) / h, 1.0); /* aspect ratio */
    float Rh = fmin(float(h) / w, 1.0);
    img = new QImage(256, 256, QImage::Format_RGBA8888);
    for (int i = 0; i < img->width(); ++i)
    {
        for (int k = 0; k < img->height(); ++k)
        {
            uint8_t *which = pix + int((float(i) / img->width()) * w / Rw) * sizeof(uint32_t)
                    + int((float(k) / img->height()) * h / Rh) * sizeof(uint32_t) * w;

            img->setPixel(i, k, qRgba(which[0], which[1], which[2], which[3]));

            /* clear any pixels sampled outside the aspect ratio */
            if (float(i) / img->width() >= Rw || float(k) / img->height() >= Rh)
                img->setPixel(i, k, qRgba(0, 0, 0, 0));
        }
    }
    ui->imagePreview->setPixmap(QPixmap::fromImage(*img));
    ui->imagePreview->adjustSize();

    /* page stats */
    *buf = '\0';
    append(buf, "%d sprites\n", sprites);
    append(buf, "%.2f%% occupancy", occupancy * 100);
    ui->pageStats->setText(buf);
    ui->pageStatsHeader->setVisible(true);

    /* global stats */
    *buf = '\0';
    append(buf, "Packed %d sprites\n", stats.sprites);
    append(buf, "Omitted %d duplicates\n", stats.duplicates);
    ui->globalStats->setText(buf);
    ui->globalStatsHeader->setVisible(true);
}

void MainWindow::on_cbLongname_stateChanged(int arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_textPrefix_textEdited(const QString &arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::on_textRegex_textEdited(const QString &arg1)
{
    (void)arg1;
    SettingsChange();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QUrl url;
    QString str;

    if (!mimeData->hasUrls())
        return;

    /* complain if the user dropped more than one */
    if (mimeData->urls().count() != 1)
    {
        QMessageBox::warning(global, "Warning", "Please drop only one directory.");
        return;
    }

    /* convert dropped URL to local file path, ignoring non-local files */
    url = mimeData->urls().at(0);
    if (!url.isLocalFile())
        return;
    str = url.toLocalFile();

    /* complain if user dropped something other than a directory */
    if (!QFileInfo(str).isDir())
    {
        QMessageBox::warning(global, "Warning", QString("'%1' is not a directory.").arg(str));
        return;
    }

    //success_overload(str.toUtf8().constData());
    setFolderText(str);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}
