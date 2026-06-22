namespace SolutionUpdater
{
    partial class MainWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.openSLNFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.OpenSolutionButton = new System.Windows.Forms.Button();
            this.currentSolutionFilename = new System.Windows.Forms.Label();
            this.startUpdate = new System.Windows.Forms.Button();
            this.backupEditedFilesCheckbox = new System.Windows.Forms.CheckBox();
            this.groupBoxPlatform = new System.Windows.Forms.GroupBox();
            this.checkBoxXbox = new System.Windows.Forms.CheckBox();
            this.checkBoxPC = new System.Windows.Forms.CheckBox();
            this.groupBoxPlatform.SuspendLayout();
            this.SuspendLayout();
            // 
            // openSLNFileDialog
            // 
            this.openSLNFileDialog.Filter = "Solution files (*.sln)|*.sln";
            this.openSLNFileDialog.Title = "Select Solution file to Update";
            // 
            // OpenSolutionButton
            // 
            this.OpenSolutionButton.Location = new System.Drawing.Point(19, 33);
            this.OpenSolutionButton.Name = "OpenSolutionButton";
            this.OpenSolutionButton.Size = new System.Drawing.Size(75, 32);
            this.OpenSolutionButton.TabIndex = 0;
            this.OpenSolutionButton.Text = "Open...";
            this.OpenSolutionButton.UseVisualStyleBackColor = true;
            this.OpenSolutionButton.Click += new System.EventHandler(this.OpenSolutionButton_Click);
            // 
            // currentSolutionFilename
            // 
            this.currentSolutionFilename.AutoSize = true;
            this.currentSolutionFilename.BackColor = System.Drawing.SystemColors.GradientActiveCaption;
            this.currentSolutionFilename.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.currentSolutionFilename.Location = new System.Drawing.Point(101, 39);
            this.currentSolutionFilename.MinimumSize = new System.Drawing.Size(400, 20);
            this.currentSolutionFilename.Name = "currentSolutionFilename";
            this.currentSolutionFilename.Size = new System.Drawing.Size(400, 20);
            this.currentSolutionFilename.TabIndex = 1;
            this.currentSolutionFilename.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // startUpdate
            // 
            this.startUpdate.Location = new System.Drawing.Point(184, 122);
            this.startUpdate.Name = "startUpdate";
            this.startUpdate.Size = new System.Drawing.Size(88, 30);
            this.startUpdate.TabIndex = 2;
            this.startUpdate.Text = "Start Update";
            this.startUpdate.UseVisualStyleBackColor = true;
            this.startUpdate.Click += new System.EventHandler(this.StartUpdate_Click);
            // 
            // backupEditedFilesCheckbox
            // 
            this.backupEditedFilesCheckbox.AutoSize = true;
            this.backupEditedFilesCheckbox.Checked = true;
            this.backupEditedFilesCheckbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.backupEditedFilesCheckbox.Location = new System.Drawing.Point(184, 96);
            this.backupEditedFilesCheckbox.Name = "backupEditedFilesCheckbox";
            this.backupEditedFilesCheckbox.Size = new System.Drawing.Size(125, 17);
            this.backupEditedFilesCheckbox.TabIndex = 3;
            this.backupEditedFilesCheckbox.Text = "Backup edited files ?";
            this.backupEditedFilesCheckbox.UseVisualStyleBackColor = true;
            // 
            // groupBoxPlatform
            // 
            this.groupBoxPlatform.Controls.Add(this.checkBoxPC);
            this.groupBoxPlatform.Controls.Add(this.checkBoxXbox);
            this.groupBoxPlatform.Location = new System.Drawing.Point(382, 72);
            this.groupBoxPlatform.Name = "groupBoxPlatform";
            this.groupBoxPlatform.Size = new System.Drawing.Size(119, 80);
            this.groupBoxPlatform.TabIndex = 4;
            this.groupBoxPlatform.TabStop = false;
            this.groupBoxPlatform.Text = "Platforms to Update";
            // 
            // checkBoxXbox
            // 
            this.checkBoxXbox.AutoSize = true;
            this.checkBoxXbox.Checked = true;
            this.checkBoxXbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxXbox.Location = new System.Drawing.Point(18, 24);
            this.checkBoxXbox.Name = "checkBoxXbox";
            this.checkBoxXbox.Size = new System.Drawing.Size(73, 17);
            this.checkBoxXbox.TabIndex = 0;
            this.checkBoxXbox.Text = "Xbox One";
            this.checkBoxXbox.UseVisualStyleBackColor = true;
            // 
            // checkBoxPC
            // 
            this.checkBoxPC.AutoSize = true;
            this.checkBoxPC.Checked = true;
            this.checkBoxPC.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxPC.Location = new System.Drawing.Point(18, 47);
            this.checkBoxPC.Name = "checkBoxPC";
            this.checkBoxPC.Size = new System.Drawing.Size(70, 17);
            this.checkBoxPC.TabIndex = 1;
            this.checkBoxPC.Text = "Windows";
            this.checkBoxPC.UseVisualStyleBackColor = true;
            // 
            // MainWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(524, 173);
            this.Controls.Add(this.groupBoxPlatform);
            this.Controls.Add(this.backupEditedFilesCheckbox);
            this.Controls.Add(this.startUpdate);
            this.Controls.Add(this.currentSolutionFilename);
            this.Controls.Add(this.OpenSolutionButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "MainWindow";
            this.Text = "Solution Updater";
            this.groupBoxPlatform.ResumeLayout(false);
            this.groupBoxPlatform.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.OpenFileDialog openSLNFileDialog;
        private System.Windows.Forms.Button OpenSolutionButton;
        private System.Windows.Forms.Label currentSolutionFilename;
        private System.Windows.Forms.Button startUpdate;
        private System.Windows.Forms.CheckBox backupEditedFilesCheckbox;
        private System.Windows.Forms.GroupBox groupBoxPlatform;
        private System.Windows.Forms.CheckBox checkBoxXbox;
        private System.Windows.Forms.CheckBox checkBoxPC;
    }
}

