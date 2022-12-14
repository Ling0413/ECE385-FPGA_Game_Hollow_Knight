/************************************************************************
Avalon-MM Interface for AES Decryption IP Core

Dong Kai Wang, Fall 2017

For use with ECE 385 Experiment 9
University of Illinois ECE Department

Register Map:

 0-3 : 4x 32bit AES Key
 4-7 : 4x 32bit AES Encrypted Message
 8-11: 4x 32bit AES Decrypted Message
   12: Not Used
	13: Not Used
   14: 32bit Start Register
   15: 32bit Done Register

************************************************************************/

module avalon_aes_interface (
	// Avalon Clock Input
	input logic CLK,
	
	// Avalon Reset Input
	input logic RESET,
	
	// Avalon-MM Slave Signals
	input  logic AVL_READ,					// Avalon-MM Read
	input  logic AVL_WRITE,					// Avalon-MM Write
	input  logic AVL_CS,						// Avalon-MM Chip Select
	input  logic [3:0] AVL_BYTE_EN,		// Avalon-MM Byte Enable
	input  logic [3:0] AVL_ADDR,			// Avalon-MM Address
	input  logic [31:0] AVL_WRITEDATA,	// Avalon-MM Write Data
	output logic [31:0] AVL_READDATA,	// Avalon-MM Read Data
	
	// Exported Conduit
	output logic [31:0] EXPORT_DATA		// Exported Conduit Signal to LEDs
);
	
	// All registers
	logic [31:0] Regs_File [16];
	
	// Register file
	always_ff @ (posedge CLK) begin
			if (RESET) begin
					for (int i = 0; i < 16; i++) begin
							Regs_File[i] <= 32'h0;
					end
			end
			else if (AVL_WRITE && AVL_CS) begin
					unique case (AVL_BYTE_EN)
							4'b1100 : Regs_File[AVL_ADDR] <= {AVL_WRITEDATA[31:16], Regs_File[AVL_ADDR][15:0]};
							4'b0011 : Regs_File[AVL_ADDR] <= {Regs_File[AVL_ADDR][31:16], AVL_WRITEDATA[15:0]};
							4'b1000 : Regs_File[AVL_ADDR] <= {AVL_WRITEDATA[31:24], Regs_File[AVL_ADDR][23:0]};
							4'b0100 : Regs_File[AVL_ADDR] <= {Regs_File[AVL_ADDR][31:24], AVL_WRITEDATA[23:16], Regs_File[AVL_ADDR][15:0]};
							4'b0010 : Regs_File[AVL_ADDR] <= {Regs_File[AVL_ADDR][31:16], AVL_WRITEDATA[15:8], Regs_File[AVL_ADDR][7:0]};
							4'b0001 : Regs_File[AVL_ADDR] <= {Regs_File[AVL_ADDR][31:8], AVL_WRITEDATA[7:0]};
							default Regs_File[AVL_ADDR] <= AVL_WRITEDATA;
					endcase
			end
			else begin
					for (int i = 0; i < 16; i++) begin
							Regs_File[i] <= Regs_File[i];
					end
			end
	end
	
	
	// Perform read operation
	always_comb begin
			if (AVL_READ && AVL_CS) begin
					AVL_READDATA = Regs_File[AVL_ADDR];
			end
			else begin
					AVL_READDATA = 32'h0;
			end
	end
	
	// Connect Exported Conduit Signal
	assign EXPORT_DATA = {Regs_File[4][31:16], Regs_File[7][15:0]};

endmodule
